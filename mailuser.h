/* mailuser.h
 * Handles authentication and mail data for an email system
 * Author  : Jonatan Schroeder
 * Modified: Nov 5, 2021
 *
 * Modified by: Norm Hutchinson
 * Modified: Mar 5, 2022
 */

#ifndef _MAILUSER_H_
#define _MAILUSER_H_

#include <stdio.h>

#define MAX_USERNAME_SIZE 255
#define MAX_PASSWORD_SIZE 255

typedef struct user_list *user_list_t;
typedef struct mail_item *mail_item_t;
typedef struct mail_list *mail_list_t;

int 	    is_valid_user(const char *username, const char *password);

user_list_t user_list_create(void);
void	    user_list_add(user_list_t *list, const char *username);
void 	    user_list_destroy(user_list_t list);
int 	    user_list_len(user_list_t list);

void 	    save_user_mail(const char *basefile, user_list_t users);

mail_list_t load_user_mail(const char *username);
int         mail_list_destroy(mail_list_t list);
int         mail_list_length(mail_list_t list, int includedeleted);
mail_item_t mail_list_retrieve(mail_list_t list, unsigned int pos);
size_t      mail_list_size(mail_list_t list);
int         mail_list_undelete(mail_list_t list);

size_t      mail_item_size(mail_item_t item);
FILE       *mail_item_contents(mail_item_t item);
void        mail_item_delete(mail_item_t item);

#endif
