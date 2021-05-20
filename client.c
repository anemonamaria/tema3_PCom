#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdbool.h>
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include "helpers.h"
#include "parson.h"
#include "requests.h"

#define REGISTER "/api/v1/tema/auth/register"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"

char *rcv_post_request(int socket, char host[16], char *command, char *login[1], char *token) {
	char *message = compute_post_request(host, command, "application/json", login, 1, NULL, 0, token);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

char *rcv_get_request(int socket, char host[16], char *command, char *token, char *cookies[1], char *get_delete) {
	char *message = compute_get_request(host, command, NULL, cookies, 1, token, get_delete);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

int main(int argc, char *argv[]) {
	int exit = 1, logged = 0, library = 0;
	char host[16] = "34.118.48.238";
	int port = 8080;
	int socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

	char *login[1], *cookies[1], cookie[100], token[BUFLEN], *book[1];


	char comanda[25];
	while(exit) {
		fgets(comanda, 25, stdin);

		if(strncmp(comanda, "exit", 4) == 0) {
			exit = 0;

			break;
		} else {
			socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

			if (strncmp(comanda, "register", 8) == 0) {
				char username[BUFLEN], password[BUFLEN];

				printf("username=");
				scanf("%s", username);
				printf("password=");
				scanf("%s", password);

				JSON_Value *value = json_value_init_object();
				JSON_Object *object = json_value_get_object(value);
				json_object_set_string(object, "username", username);
				json_object_set_string(object, "password", password);
				login[0] = json_serialize_to_string_pretty(value);

				if(strstr(rcv_post_request(socket, host, REGISTER, login, NULL), "is taken") != NULL) {
					printf("TAKEN USERNAME! Please try another one.\n");
				} else {
					printf("DONE! You are now registred.\n");
				}

			}

			else if (strncmp(comanda, "login", 5) == 0) {
				char username[BUFLEN], password[BUFLEN];
				logged = 1;

				printf("username=");
				scanf("%s", username);
				printf("password=");
				scanf("%s", password);

				JSON_Value *value = json_value_init_object();
				JSON_Object *object = json_value_get_object(value);
				json_object_set_string(object, "username", username);
				json_object_set_string(object, "password", password);
				login[0] = json_serialize_to_string_pretty(value);

				char *cookie_ptr = strstr(rcv_post_request(socket, host, LOGIN, login, NULL), "Set-Cookie: ");

				if (cookie_ptr == NULL) {
					printf("FAILED!\n");
					logged = 0;
					library = 0;
					continue;
				}

				strtok(cookie_ptr, ";");
				cookie_ptr += 12;
				strcpy(cookie, cookie_ptr);
				cookies[0] = cookie_ptr;

				if(cookie != NULL) {
					printf("Logged in!\n");
				}

			}

			else if (strncmp(comanda, "enter_library", 13) == 0) {
				if(logged) {
					library = 1;

					char *ptr = strstr(rcv_get_request(socket, host, ACCESS, token, cookies, "get"), "token");
					if (ptr == NULL) {
						printf("NO ACCESS!\n");
						library = 0;
					} else {
						ptr += 8;
						memset(token, 0, BUFLEN);
						strcpy(token, ptr);
						token[strlen(token) - 2] = '\0';
					}

				} else {
					printf("Please log in.\n");
				}
			}

			else if (strncmp(comanda, "get_books", 9) == 0) {
				if (library == 1) {
					printf("%s\n", strstr(rcv_get_request(socket, host, BOOKS, token, cookies, "get"), "["));
				} else {
					printf("Please enter the library!\n");
				}
			}
			else if (strncmp(comanda, "get_book", 8) == 0) {
				if (library == 1) {
					char route[BUFLEN];
					int book_id = 0;

					printf("id=");
					scanf("%d", &book_id);
					if(book_id < 0) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("id=");
						scanf("%d", &book_id);
					}

					sprintf(route, "%s/%d", BOOKS, book_id);

					char *response = rcv_get_request(socket, host, route, token, cookies, "get");

					if (strstr(response, "No book was found!") != NULL) {
						printf("NO BOOK! Entered id is not valid!\n");
					} else {
						char *book = strstr(response, "[");
						printf("%s\n", book);
					}

				} else {
					printf("Please enter the library!\n");
				}
			}
			else if (strncmp(comanda, "add_book", 8) == 0) {
				if (library == 1) {
					char title[100], author[100], genre[100], publisher[100];
					int pages;

					printf("title=");
					scanf("%s", title);
					if(strcmp(title, "") == 0) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("title=");
						scanf("%s", title);
					}
					printf("author=");
					scanf("%s", author);
					if(strcmp(author, "") == 0) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("author=");
						scanf("%s", author);
					}
					printf("genre=");
					scanf("%s", genre);
					if(strcmp(genre, "") == 0) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("genre=");
						scanf("%s", genre);
					}
					printf("publisher=");
					scanf("%s", publisher);
					if(strcmp(publisher, "") == 0) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("publisher=");
						scanf("%s", publisher);
					}
					printf("pages=");
					scanf("%d", &pages);
					if(pages < 1) {
						printf("WRONG FORMAT! Please try again.\n");
						printf("pages=");
						scanf("%d", &pages);
					}

					char pages_string[BUFLEN];
					sprintf(pages_string, "%d", pages);

					JSON_Value *value = json_value_init_object();
					JSON_Object *object = json_value_get_object(value);
					json_object_set_string(object, "title", title);
					json_object_set_string(object, "author", author);
					json_object_set_string(object, "genre", genre);
					json_object_set_string(object, "page_count", pages_string);
					json_object_set_string(object, "publisher", publisher);
					book[0] = json_serialize_to_string_pretty(value);

					rcv_post_request(socket, host, BOOKS, book, token);
				} else {
					printf("Please enter the library!\n");
				}
			}
			else if (strncmp(comanda, "delete_book", 11) == 0) {
				if (library == 1) {
					char route[BUFLEN];
					int book_id = 0;

					printf("id=");
					scanf("%d", &book_id);
					if(book_id < 0) {
						printf("WRONG FORMAT! Please try again.\n");
						scanf("%d", &book_id);
					}

					sprintf(route, "%s/%d", BOOKS, book_id);

					char *delete = strstr(rcv_get_request(socket, host, route, token, cookies, "delete"), "No book was deleted!");
					if (delete != NULL) {
						printf("NO BOOK! Entered id is not valid!\n");
					}

				} else {
					printf("Please enter the library!\n");
				}
			}

			else if (strncmp(comanda, "logout", 6) == 0) {
				if (logged == 1) {
					rcv_get_request(socket, host, LOGOUT, token, cookies, "get");
					logged = 0;
					library = 0;

				} else {
					printf("Not logged in!\n");
				}
			}
			close_connection(socket);
		}
	}
}