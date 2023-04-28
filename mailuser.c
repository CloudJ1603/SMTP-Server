/* mailuser.c
 * Handles authentication and mail data for an email system
 * Author  : Jonatan Schroeder
 * Modified: Nov 5, 2021
 *
 * Modified by: Norm Hutchinson
 * Modified: Mar 5, 2022
 */

#include "mailuser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#define USER_FILE_NAME "users.txt"
#define MAIL_BASE_DIRECTORY "mail.store"
#define MAIL_FILE_SUFFIX ".mail"

struct user_list {
    char *user;
    struct user_list *next;
};

struct mail_item {
    char file_name[2 * NAME_MAX];
    size_t file_size;
    int deleted;
};

struct mail_list {
    struct mail_item item;
    struct mail_list *next;
};

/** Internal function that opens the users file list. If file has been
 *  opened before, rewinds the pointer to beginning of the file.
 * 
 *  Returns: file pointer for users file, or NULL if file cannot be opened.
 */
static FILE *user_file_list(void) {

    static FILE *file_ptr = NULL;
    if (!file_ptr)
        file_ptr = fopen(USER_FILE_NAME, "r+");
    if (file_ptr)
        rewind(file_ptr);
    return file_ptr;
}
/** Checks if the user name is valid. If password is supplied, also
 *  checks if the password matches the user name. The username check
 *  ignores case (i.e., upper-case and lower-case letters are
 *  considered equivalent), so a username like 'ADMIN' is considered
 *  equivalent to 'admin'. The password check, if performed, is
 *  case-sensitive (i.e., upper-case and lower-case letters are
 *  considered different).
 *  
 *  Parameters: username: Non-NULL name of the user to check.
 *              password: Plain-text password to check. If NULL, will
 *                        check only that the username exists.
 *
 *  Returns: if the password is NULL, returns non-zero (true) if the
 *           username exists, and zero (false) otherwise. If the
 *           password is not NULL, returns non-zero (true) if the
 *           username exists and the password matches the user's
 *           password, and zero (false) otherwise.
 */
int is_valid_user(const char *username, const char *password) {
    FILE *file_ptr = user_file_list();
    if (!file_ptr) return 0;
    char user_file[MAX_USERNAME_SIZE+1];
    char pw_file[MAX_PASSWORD_SIZE+1];
    while (fscanf(file_ptr, "%s%s", user_file, pw_file) == 2) {
        if (!strcasecmp(username, user_file))
            return password == NULL || !strcmp(password, pw_file);
    }
    return 0;
}

/** Creates a new, empty, list of users.
 * 
 *  Returns: A user_list_t object with no users.
 */
user_list_t user_list_create(void) {
    return NULL;
}

/** Return the length of a list of users.
 *
 * Parameters: list: address of the list of users
 *
 * Returns: the number of users in the list
 *
 **/
int user_list_len(user_list_t list) {
    int len = 0;
    while (list != NULL) {
	len++;
	list = list->next;
    }
    return len;
}

/** Adds a user name to a list of users.
 *  
 *  Parameters: list: address of the list of users to be modified.
 *              username: Name of the user to be added. The name will
 *                        be copied to a new buffer, so the caller is
 *                        free to use a string that will be modified
 *                        later.
 */
void user_list_add(user_list_t *list, const char *username) {
    user_list_t new_list = malloc(sizeof(struct user_list));
    new_list->user = strdup(username);
    new_list->next = *list;
    *list = new_list;
}

/** Frees all memory used by a list of users.
 *
 * Parameters: list: list of users to be freed.
 */
void user_list_destroy(user_list_t list) {
    while (list) {
        user_list_t next = list->next;
        free(list->user);
        free(list);
        list = next;
    }
}

/** Saves a new email message into the mail storage for a list of
 *  users.
 *
 *  This function uses hard links to create the files based on an
 *  existing temporary file. It assumes the temporary file is in the
 *  same file system as the newly created files. Typically, saving the
 *  temporary file in a local directory (where the executable is
 *  running) is enough for this to work.
 *
 *  Parameters: basefile: Name of a temporary file containing the
 *                        contents of the email message.
 *              users: List of recipient users to the message.
 */
void save_user_mail(const char *basefile, user_list_t users) {
  
    char mail_file[2 * NAME_MAX + 1];
  
    // Create base directory if it doesn't exist yet (error ignored)
    mkdir(MAIL_BASE_DIRECTORY, 0777);
  
    for (; users; users = users->next) {
    
        // Create a directory for the user if it doesn't exist yet. If it
        // exists mkdir will return an error, which is ignored.
        int i = 0;
        sprintf(mail_file, "%s/%s", MAIL_BASE_DIRECTORY, users->user);
        mkdir(mail_file, 0777);
    
        // Tries to create a file called 0.mail, if it exists tries 1.mail, and so on
        do {
            sprintf(mail_file, "%s/%s/%d" MAIL_FILE_SUFFIX, MAIL_BASE_DIRECTORY, users->user, i++);
        } while (link(basefile, mail_file) < 0 && errno == EEXIST);
    }
}

/** Reads the list of available email messages for a username, based
 *  on existing email files created using save_user_mail (or
 *  equivalent). Only file names and sizes are loaded into memory, the
 *  messages themselves are not kept in memory. If the user does not
 *  exist or does not have any messages, an empty list is returned.
 *
 *  Parameters: username: Name of the user whose email messages should
 *                        be retrieved.
 *
 *  Returns: A mail_list_t object containing a list of email messages
 *           available for the provided username.
 */
mail_list_t load_user_mail(const char *username) {
  
    char filename[2 * NAME_MAX + 1];
    sprintf(filename, "%s/%s", MAIL_BASE_DIRECTORY, username);
  
    DIR *dir = opendir(filename);
    if (!dir) return NULL;
  
    struct stat file_stat;
    struct dirent *dir_entry;
    const size_t suflen = strlen(MAIL_FILE_SUFFIX);
    struct mail_list *list = NULL;
  
    while ((dir_entry = readdir(dir)) != NULL) {
    
        if (// Check if it's a regular file (not a directory)
            dir_entry->d_type == DT_REG &&
            // Check if the filename is big enough to contain the suffix
            strlen(dir_entry->d_name) > suflen &&
            // Check if the filename ends with the mail suffix
            !strcmp(dir_entry->d_name + strlen(dir_entry->d_name) - suflen, MAIL_FILE_SUFFIX)) {
      
            struct mail_list *node = malloc(sizeof(struct mail_list));
            sprintf(node->item.file_name, "%s/%s/%s",
                    MAIL_BASE_DIRECTORY, username, dir_entry->d_name);
      
            if (stat(node->item.file_name, &file_stat) < 0) {
                free(node);
                continue;
            }
      
            node->item.file_size = file_stat.st_size;
            node->item.deleted = 0;
            struct mail_list *next = list;
            struct mail_list **prev = &list;
            while (next && strcmp(node->item.file_name, next->item.file_name) > 0) {
                prev = &next->next;
                next = next->next;
            }
            node->next = next;
            *prev = node;
        }
    }
    closedir(dir);
    return list;
}

/** Frees all memory used by a list of emails. Also deletes any files
 *  marked to be deleted.
 *
 *  Parameters: list: List of emails to be deleted.
 *  Return:     number of errors, if any
 */
int mail_list_destroy(mail_list_t list) {
    int errors = 0;
    while (list) {
        if (list->item.deleted) {
            if (unlink(list->item.file_name) < 0) {
                errors++;
            }
        }
        mail_list_t next = list->next;
        free(list);
        list = next;
    }
    return errors;
}

/** Returns the number of email messages available in a list of
 *  emails, not counting messages marked for deletion (e.g., if there
 *  are 4 messages, and the second is marked as deleted,
 *  mail_list_length will return 3).  If the includedeleted parameter is true,
 *  then deleted messages are included in the count, so in the previous
 *  example, the return value would be 4. 
 *
 *  Parameters: list: List of emails to be assessed.
 *              includedeleted: should deleted messages be included in the count
 *
 *  Returns: Number of non-deleted messages in list.
 */
int mail_list_length(mail_list_t list, int includedeleted) {
    int rv = 0;
    while (list) {
        if (includedeleted || !list->item.deleted) rv++;
        list = list->next;
    }
    return rv;
}

/** Returns the email message object at a specific position in a list
 *  of emails. The position starts at 0 for the first message (i.e.,
 *  calling this function with pos set to zero will return the first
 *  message). Messages marked for deletion are also counted in the
 *  position count; for example, if message at position 1 is marked
 *  for deletion, then any messages at position 2 and beyond continue
 *  to have their position numbers unmodified.
 *
 *  Parameters: list: List of emails to be assessed.
 *              pos: Zero-based position of the message to be
 *                   retrieved.
 *
 *  Returns: mail_item_t object corresponding to the message, or NULL
 *           if the position is invalid or the message is marked as
 *           deleted.
 */
mail_item_t mail_list_retrieve(mail_list_t list, unsigned int pos) {
  
    while (list) {
        if (!pos--)
            return list->item.deleted ? NULL : &list->item;
        list = list->next;
    }
  
    return NULL;
}

/** Returns the total amount of bytes in all email messages in a list
 *  of emails, not counting messages marked for deletion.
 *
 *  Parameters: list: List of emails to be assessed.
 *
 *  Returns: Total size for all non-deleted messages in list.
 */
size_t mail_list_size(mail_list_t list) {
    size_t rv = 0;
    while (list) {
        rv += list->item.deleted ? 0 : list->item.file_size;
        list = list->next;
    }
    return rv;
}

/** Returns the total amount of bytes in an email message.
 *
 *  Parameters: item: Email message to be assessed.
 *
 *  Returns: Size, in bytes, of an email message.
 */
size_t mail_item_size(mail_item_t item) {
    return item->file_size;
}

/** Returns a file pointer that can be used to read the contents of an
 *  email message. The caller is responsible for closing the file
 *  using the `fclose()` function once the data is no longer needed.
 *
 *  Parameters: item: Email message to be retrieved.
 *
 *  Returns: FILE * object, or NULL in case of error retrieving the
 *           contents.
 */
FILE *mail_item_contents(mail_item_t item) {
    return fopen(item->file_name, "r");
}

/** Marks a message for deletion in the internal email list. Does not
 *  actually delete the email contents, as a reset call may still
 *  recover the email message. The message is only deleted when the
 *  email list is destroyed (through mail_list_destroy).
 *
 *  Parameters: item: Email message to be marked for deletion.
 */
void mail_item_delete(mail_item_t item) {
    item->deleted = 1;
}

/** Marks all deleted messages in a list as no longer deleted.
 *
 *  Parameters: list: Email list to be assessed.
 *
 *  Returns: Number of recovered messages.
 */
int mail_list_undelete(mail_list_t list) {
  
    int rv = 0;
  
    while (list) {
        rv += list->item.deleted;
        list->item.deleted = 0;
        list = list->next;
    }
  
    return rv;
}
