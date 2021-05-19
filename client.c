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

int main(int argc, char *argv[]) {
	int exit = 1, logged = 0, library = 0;
	char host[16] = "34.118.48.238";
	int port = 8080;
	int socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

	char *login[1], *message, *response, *cookies[1], cookie[100], token[BUFLEN];


	char comanda[25];
	while(exit) {
		fgets(comanda, 25, stdin);

		if(strncmp(comanda, "exit", 4) == 0) {
			exit = 0;
			break;
			// continue;
		}

		else if (strncmp(comanda, "register", 8) == 0) {
			char username[BUFLEN], password[BUFLEN];
			socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

			printf("username=");
			scanf("%s", username);
			printf("password=");
			scanf("%s", password);

			JSON_Value *value = json_value_init_object();
        	JSON_Object *object = json_value_get_object(value);
        	json_object_set_string(object, "username", username);
        	json_object_set_string(object, "password", password);
        	login[0] = json_serialize_to_string_pretty(value);

			message = compute_post_request(host, REGISTER, "application/json", login, 1, NULL, 0, NULL);
			send_to_server(socket, message);

			response  =receive_from_server(socket);
			//printf("%s\n", response);
			char *taken = strstr(response, "taken");

			if(taken != NULL) {
				printf("TAKEN USERNAME? Please try another one.\n");
			} else {
				printf("DONE! You are now registred.\n");
			}
			close_connection(socket);
			//break;
			continue;

		}

		else if (strncmp(comanda, "login", 5) == 0) {
		//	if(logged ==0 ) {
			char username[BUFLEN], password[BUFLEN];
			socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
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

			message = compute_post_request(host, LOGIN, "application/json", login, 1, NULL, 0, NULL);
			send_to_server(socket, message);

			response = receive_from_server(socket);
			//printf("%s\n", response);
			char *cookie_ptr = strstr(response, "Set-Cookie: ");

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

			close_connection(socket);
				//break;
			continue;
			// } else {
			// 	printf("Already logged in!\n");
			// 	continue;
			// }

		}

		else if (strncmp(comanda, "enter_library", 13) == 0) {
			if(logged) {
				socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
				message = compute_get_request(host, ACCESS, NULL, cookies, 1, NULL);
				send_to_server(socket, message);
				response = receive_from_server(socket);
				library = 1;

				char *ptr = strstr(response, "token");
				if (ptr == NULL) {
					printf("NO ACCESS!\n");
					library = 0;
				} else {
					ptr += 8;
					memset(token, 0, BUFLEN);
					strcpy(token, ptr);
					token[strlen(token) - 2] = '\0';
					//printf("Access permited!\n");
				}

				close_connection(socket);
			} else {
				printf("Please log in.\n");
			}
			continue;
		}
		else if (strncmp(comanda, "get_books", 9) == 0) {
			if (library == 1) {
				socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
				message = compute_get_request(host, BOOKS, NULL, cookies, 1, token);
				if (token == NULL) {
					printf("ERROR!\n");
					continue;
				}
				send_to_server(socket, message);
				response = receive_from_server(socket);

				printf("%s\n", strstr(response, "["));
			} else {
				printf("Please enter the library!\n");
			}
		}
		else if (strncmp(comanda, "get_book", 8) == 0) {

		}
		else if (strncmp(comanda, "add_book", 8) == 0) {

		}
		else if (strncmp(comanda, "delete_book", 11) == 0) {

		}
		else if (strncmp(comanda, "logout", 6) == 0) {

		}
	}
}