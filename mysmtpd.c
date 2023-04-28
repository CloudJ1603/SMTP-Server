#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#include <regex.h>

// Regular expression pattern to match an email address
const char* email_regex = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";

#define MAX_LINE_LENGTH 1024

typedef enum state {
    Undefined,
    // TODO: Add additional states as necessary
    DISCONNECTED,
    CONNECTED,
    HELO_SENT,
    AUTH_SENT,
    MAIL_FROM_SENT,
    RCPT_TO_SENT,
    DATA_SENT,
    QUIT_SENT
} State;

typedef struct smtp_state {
    int fd;
    net_buffer_t nb;
    char recvbuf[MAX_LINE_LENGTH + 1];
    char *words[MAX_LINE_LENGTH];
    int nwords;
    State state;
    struct utsname my_uname;
    // TODO: Add additional fields as necessary
    user_list_t userList;
} smtp_state;
    
static void handle_client(int fd);
int is_email_valid(char* email);

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
  
    run_server(argv[1], handle_client);
  
    return 0;
}

// syntax_error returns
//   -1 if the server should exit
//    1  otherwise
int syntax_error(smtp_state *ms) {
    if (send_formatted(ms->fd, "501 %s\r\n", "Syntax error in parameters or arguments") <= 0) return -1;
    return 1;
}

// checkstate returns
//   -1 if the server should exit
//    0 if the server is in the appropriate state
//    1 if the server is not in the appropriate state
int checkstate(smtp_state *ms, State s) {
    if (ms->state != s) {
	if (send_formatted(ms->fd, "503 %s\r\n", "Bad sequence of commands") <= 0) return -1;
	return 1;
    }
    return 0;
}

// All the functions that implement a single command return
//   -1 if the server should exit
//    0 if the command was successful
//    1 if the command was unsuccessful

int do_quit(smtp_state *ms) {
    dlog("Executing quit\n");
    // TODO: Implement this function
    send_formatted(ms->fd, "221 Service closing transmission channel\r\n");
    ms->state = QUIT_SENT;
    return -1;
}

int do_helo(smtp_state *ms) {
    dlog("Executing helo\n");
    // TODO: Implement this function
    if (send_formatted(ms->fd, "250 %s\r\n", ms->my_uname.nodename) > 0){
        ms->state = HELO_SENT;
        ms->userList = (user_list_t) user_list_create();
        return 0;
    }
    return 1;
}

int do_rset(smtp_state *ms) {
    dlog("Executing rset\n");
    // TODO: Implement this function
    ms->state = HELO_SENT;
    if(ms->userList != NULL) {
        user_list_destroy(ms->userList);
        ms->userList = NULL;
    }
    send_formatted(ms->fd, "250 State reset\r\n");
    return 0;
}

int do_mail(smtp_state *ms) {
    dlog("Executing mail\n");
    // TODO: Implement this function 

    
    if(checkstate(ms, HELO_SENT)) {    // check state
        return 1;
    }

    if(ms->nwords == 2) {     //
        char* ptr = ms->words[1];
        if(strncmp(ptr, "FROM:", 5) == 0) {    // check from 
            ptr += 5;
            if (ptr[0] == '<' && ptr[strlen(ptr) - 1] == '>') {   // check '<' and '>'
                char* email = trim_angle_brackets(ptr);
                if(is_email_valid(email)) {
                    send_formatted(ms->fd, "250 Requested mail action ok, completed\r\n");
                    ms->state = MAIL_FROM_SENT;

                    ms-> userList = user_list_create();

                    return 0;
                }
            } 
        } 
    } 
        
    
    return syntax_error(ms);

}     

int do_rcpt(smtp_state *ms) {
    dlog("Executing rcpt\n");
    // TODO: Implement this function
    // check state
    if(ms->userList == NULL) {
        if(checkstate(ms, MAIL_FROM_SENT) != 0) {
            return 1;
        }
    } else {
        if(checkstate(ms, RCPT_TO_SENT) != 0) {
            return 1;
        }
    }

    if(ms->nwords == 2) {     // check word length
        char* ptr = ms->words[1];
        if(strncmp(ptr, "TO:", 3) == 0) {    // check from 
            ptr += 3;
            if (ptr[0] == '<' && ptr[strlen(ptr) - 1] == '>') {   // check '<' and '>'
                char* email = trim_angle_brackets(ptr);
                if(is_valid_user(email, NULL)) {
                    send_formatted(ms->fd, "250 Requested mail action ok, completed\r\n");
                    user_list_add(&ms->userList, email);
                    ms->state = RCPT_TO_SENT;
                } else {
                    send_formatted(ms->fd, "550 No such user - %s\r\n", ms->my_uname.nodename);
                }
                return 0;
            }
        } 
    } 

    send_formatted(ms->fd, "501 Syntax error in parameters or arguments\r\n");
    return 1;
}     

int do_data(smtp_state *ms) {
    dlog("Executing data\n");
    // TODO: Implement this function
    if (checkstate(ms, RCPT_TO_SENT) == 0) {
        // send success message
        send_formatted(ms->fd, "354 Waiting for data, finish with <CR><LF>.<CR><LF>\r\n");
        ms->state = DATA_SENT;

        char template[] = "./temp_XXXXXX";
        int ff = mkstemp(template);

        size_t len;
        while ((len = nb_read_line(ms->nb, ms->recvbuf)) >= 0) {
            // dlog("Current Data: %s\n", ms->recvbuf);
            // Remove CR, LF and other space characters from end of buffer
            while (isspace(ms->recvbuf[len - 1])) ms->recvbuf[--len] = 0;
            // check if finished
            if(len == 1 && ms->recvbuf[len - 1] == '.') {
                break;
            }

            //delete the first period if there are other chars followed
            if(strlen(ms->recvbuf) > 1 && strncmp(ms->recvbuf, ".", 1) == 0){
                write(ff, ms->recvbuf + 1, strlen(ms->recvbuf) - 1);
                write(ff,"\r\n",2);
            }
            else{
                write(ff, ms->recvbuf, strlen(ms->recvbuf));
                write(ff,"\r\n",2);
            }
        }
        send_formatted(ms->fd, "250 Requested mail action ok, completed\r\n");
        ms->state = RCPT_TO_SENT;
        close(ff);
        save_user_mail(template, ms->userList);
        unlink(template);
        return 0;   
    }
    return 1;
}    
      
int do_noop(smtp_state *ms) {
    dlog("Executing noop\n");
    // TODO: Implement this function
    if (send_formatted(ms->fd, "250 OK (noop)\r\n") > 0){
        return 0;
    }
    return 1;
}

int do_vrfy(smtp_state *ms) {
    dlog("Executing vrfy\n");
    // TODO: Implement this function
    if(ms->nwords == 2) {
        if(is_valid_user(ms->words[1], NULL)) {
            send_formatted(ms->fd, "250 Requested mail action ok, completed\r\n");
        } else {
            send_formatted(ms->fd, "550 No such user - %s\r\n", ms->my_uname.nodename);
        }
        return 0;
    }
    send_formatted(ms->fd, "501 Syntax error in parameters or arguments\r\n");
    return 1;
    
}

void handle_client(int fd) {
  
    size_t len;
    smtp_state mstate, *ms = &mstate;
  
    ms->fd = fd;
    ms->nb = nb_create(fd, MAX_LINE_LENGTH);
    ms->state = Undefined;
    uname(&ms->my_uname);
    
    if (send_formatted(fd, "220 %s Service ready\r\n", ms->my_uname.nodename) <= 0) return;

  
    while ((len = nb_read_line(ms->nb, ms->recvbuf)) >= 0) {
        if (ms->recvbuf[len - 1] != '\n') {
            // command line is too long, stop immediately
            send_formatted(fd, "500 Syntax error, command unrecognized\r\n");
            break;
        }
        if (strlen(ms->recvbuf) < len) {
            // received null byte somewhere in the string, stop immediately.
            send_formatted(fd, "500 Syntax error, command unrecognized\r\n");
            break;
        }
    
        // Remove CR, LF and other space characters from end of buffer
        while (isspace(ms->recvbuf[len - 1])) ms->recvbuf[--len] = 0;
        
        dlog("Command is %s\n", ms->recvbuf);
        
        // Split the command into its component "words"
        ms->nwords = split(ms->recvbuf, ms->words);
        char *command = ms->words[0];
        
        if (!strcasecmp(command, "QUIT")) {
            if (do_quit(ms) == -1) break;
        } else if (!strcasecmp(command, "HELO") || !strcasecmp(command, "EHLO")) {
            if (do_helo(ms) == -1) break;
        } else if (!strcasecmp(command, "MAIL")) {
            if (do_mail(ms) == -1) break;
        } else if (!strcasecmp(command, "RCPT")) {
            if (do_rcpt(ms) == -1) break;
        } else if (!strcasecmp(command, "DATA")) {
            if (do_data(ms) == -1) break;
        } else if (!strcasecmp(command, "RSET")) {
            if (do_rset(ms) == -1) break;
        } else if (!strcasecmp(command, "NOOP")) {
            if (do_noop(ms) == -1) break;
        } else if (!strcasecmp(command, "VRFY")) {
            if (do_vrfy(ms) == -1) break;
        } else if (!strcasecmp(command, "EXPN") ||
            !strcasecmp(command, "HELP")) {
            dlog("Command not implemented \"%s\"\n", command);
            if (send_formatted(fd, "502 Command not implemented\r\n") <= 0) break;
        } else {
            // invalid command
            dlog("Illegal command \"%s\"\n", command);
            if (send_formatted(fd, "500 Syntax error, command unrecognized\r\n") <= 0) break;
        }
    }
  
    nb_destroy(ms->nb);
}


// Check if the email address is valid
int is_email_valid(char* email) {
    regex_t regex;
    int ret;

    if (regcomp(&regex, email_regex, REG_EXTENDED) != 0) {
        return 0; // Failed to compile regex
    }

    ret = regexec(&regex, email, 0, NULL, 0);
    regfree(&regex);

    if (ret != 0) {
        return 0; // Failed to match regex
    }

    return 1; // Email address is valid
}