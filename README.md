# SMTP-Server

## Learning Goal
- To learn how to program with TCP sockets in C;
- To learn how to read and implement a well-specified protocol;
- To develop your programming and debugging skills as they relate to the use of sockets in C;
- To implement the server side of a protocol;
- To develop general networking experimental skills;
- To develop a further understanding of what TCP does, and how to manage TCP connections from the server perspective.

## Overview
In this assignment you will use the Unix Socket API to construct an SMTP server, used for sending and receiving emails. The executable for this server will be called mysmtpd. Your program is to take a single argument, the TCP port the server is to listen on for client connections.

To start your assignment, download the file pa_smtp.zip. This file contains the base files required to implement your code, as well as helper functions and a Makefile.

In order to avoid problems with spam and misbehaved email messages, your servers will implement only internal email messages. Messages will be received by your SMTP server, saved locally, and then retrieved by your POP3 server. Your messages will not be relayed to other servers, and you will not receive messages from other servers. You are not allowed to implement any email forwarding mechanism, as unintended errors when handling this kind of traffic may cause the department's network to be flagged as an email spammer, and you may be subject to penalties such as account suspension or limited access to the department's resources.

Some code is already provided that checks for the command-line arguments, listens for a connection, and accepts a new connection. Servers written in C that need to handle multiple clients simultaneously use the fork system call to handle each accepted client in a separate process, which means your system will naturally be able to handle multiple clients simultaneously without interfering with each other. However, debugging server processes that call fork can be challenging (as the debugger normally sees the parent process and all the interesting activity happens in the child). To support this, the provided code disables the call to fork and executes all code in a single process unless you define the preprocessor symbol DOFORK. We strongly suggest that you define the symbol DOFORK only when your server is working perfectly without it.

Additionally, some provided functions also handle specific functionality needed for this assignment, including the ability to read individual lines from a socket and to handle usernames, passwords and email files.

## SMTP Server
The SMTP protocol, described in RFC 5321, is used by mail clients (such as Thunderbird and Outlook) to send email messages. You will implement the server side of this protocol. More specifically, you will create a server able to support, at the very least, the following commands:

HELO and EHLO (client identification)
MAIL (message initialization)
RCPT (recipient specification)
DATA (message contents)
RSET (reset transmission)
VRFY (check username)
NOOP (no operation)
QUIT (session termination)
The functionality of the commands above is listed in the RFC, with special consideration to sections 3 (intro), 3.1, 3.2, 3.3, 3.5, and 3.8. Technical details about these commands are found in section 4 (intro), and 4.1 to 4.3. You may skip over parts of these sections that cover commands and functionality not listed above. You must pay special attention to possible error messages, and make sure your code sends the correct error messages for different kinds of invalid input, such as invalid command ordering, missing or extraneous parameters, or invalid recipients.

About error codes: The SMTP specification is sometimes not terribly clear about what error codes are expected in certain erroneous situations. In particular, section 4.3.2 indicates the various error codes that are possible for each command. For example, for the VRFY command, it suggests that any of 550, 551, 553, 502, 504 are possible. The testing framework is more particular than that, and expects a smaller number of error codes in the erroneous situations that it creates. For failing VRFY commands, for example, the testing framework only accepts error code 550. It will, however, let you know what error code(s) it is expecting if you fail a test. Pay attention to what it says and you should be able to match its expectations reasonably easily.

About the EHLO command: you are not required to implement the multi-line response of an EHLO command. Responding with a single line that contains the response code and the host name is enough. To retrieve the host name, you may find the result of the system call function uname() useful.

About the VRFY command: the RFC lists two possible types of parameters for VRFY (end of section 3.5.1): a string containing just the username (without @domain), or a string containing both the username and domain (a regular email address format, e.g. john.doe@example.com). You are not required to implement the former, only the latter. In particular, you only need to return success (i.e., 250) if the username exists and error (i.e., 550) if the username does not exist. There is no need to return other results listed on the RFC as MAY implement like partial matches, ambiguous results, mailbox storage limitations, etc..

You are not required to implement commands not listed above, although a proper implementation must distinguish between commands you do not support (e.g., EXPN or HELP) from other invalid commands; The return code 502 is used for unsupported commands, while 500 is used for invalid commands. You are also not required to implement encryption or authentication.

Although you are to accept messages from any sender, your recipients must be limited to the ones supported by your system. You are to keep in your repository directory a text file called users.txt containing, in each line, a user name (as an email address) and a password, separated by a space. You are to accept only recipients that are users in this file. You do not need to submit this file for grading. For example, your users.txt file may contain the following content:

john.doe@example.com password123

mary.smith@example.com mypasswordisstrongerthanyours

edward.snowden@example.com ThisIsA100%SecureAndMemorablePassword

A sample users.txt file is included in the starter material provided.

Note that a single message may be delivered to more than one recipient, so while saving the recipients you must keep a list of recipients for the current message. Functionality for checking if a user is in the users.txt file and to create a list of users in memory is provided in the initial code of your repository.

Once a transaction for handling a message is finished, your program is to save a temporary file containing the file content of the email message, including headers. To avoid problems with further code you are encouraged to save this file in your repository directory, not in the /tmp folder. You may use functions like mkstemp to create this temporary file. Once you create this file and store the data in it, you may call the function save_user_mail, provided in your initial repo code, to save the email messages for each recipient of the message. This function will save the message as a text file inside the mail.store directory, with subdirectories for each recipient. You must delete the temporary file after the contents of the message have been saved to the user's mailboxes.

## Constrains
The provided code already provides functionality for several features you are expected to perform. You are strongly encouraged to read the comments for the provided functions before starting the implementation, as part of what you are expected to implement can be vastly simplified by using the provided code. In particular, note functions like:

**send_formatted**

sends a string to the socket and allows you to use printf-like format directives to include extra parameters in your call

**nb_read_line**

reads from the socket and buffers the received data, while returning a single line at a time (similar to fgets)

**dlog**

print a log message to the standard error stream, using printf-like formatting directives. You can turn on and off logging by assigning the variable be_verbose

**split**

split a line into parts that are separated by white space
You may assume that the functionality of these functions will be available and unmodified in the grading environment.

The provided code includes in mysmtpd.c the skeleton of a structure that you may find helpful to direct your efforts. You do not need to keep this skeleton and should feel free to change it as much as necessary. It is provided in the hopes that more direction will be valuable. Do remember that all of your changes must be confined to the mysmtpd.c file since that is the only file that you will submit for grading.

Don't try to implement this assignment all at once. Instead incrementally build and simultaneously test the solution. A suggested strategy is to:

- Start by reading the RFCs for the SMTP protocol, and listing a "normal" scenario, where a client will send one message to one recipient. Make note of the proper sequence of commands and replies to be used for this scenario.
- Have your program send the initial welcome message and immediately return.
- Get your server to start reading and parsing commands and arguments. You may find library routines like strcasecmp(), strncasecmp(), and strchr() useful. At this point just respond with a message indicating that the command was not recognized.
- Implement simple commands like QUIT and NOOP.
- Implement a straightforward sequence of commands as listed in your first item. Start simple, then move on to more complex tasks.
- Finally, identify potentially incorrect sequences of commands or arguments, and handle them accordingly. Examples may include sending a message to no recipient, etc.

## Test
For testing purposes you can use netcat (with the -C option), or use a regular email client. In particular, netcat can be very useful to test simple cases, unusual sequences of commands (e.g., a "DATA" command before a "RCPT" command), or incorrect behaviour. Make sure you test the case where multiple clients are connected simultaneously to your server. Also, test your solution in mixed case (e.g., QUIT vs quit).

If you are testing your server in a department computer, with a client in your own computer, you may need to connect to UBC's VPN, since a firewall may block incoming connections to the department computers that come from outside a UBC network.

The starter code includes a script test.sh that will run some simple tests. The input for these tests are in files named in.s.[0-9] and the expected output is in files named exp.s.[0-9]. Where the input files should result in mail being stored in the file system (which is in a directory named mail.store in your repository directory) the expected contents of those files are in the directory named exp.s.[0-9].mail.store. This script should run on macOS and Linux and under the Linux subsystem for Windows (WSL).
