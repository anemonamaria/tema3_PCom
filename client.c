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

// functie ajutatoare pentru primirea raspunsului de la server pentru 'POST REQUEST'
char *rcv_post_request(int socket, char host[16], char *command, char *login[1], char *token) {
	char *message = compute_post_request(host, command, "application/json", login, 1, NULL, 0, token);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

// functie ajutatoare pentru primirea raspunsului de la server pentru 'GET REQUEST'
char *rcv_get_request(int socket, char host[16], char *command, char *token, char *cookies[1], char *get_delete) {
	char *message = compute_get_request(host, command, NULL, cookies, 1, token, get_delete);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

// functie auxiliara pentru colectarea datelor utilizatorului
char *user() {
	char *username = calloc(100, sizeof(char));
	char *password = calloc(100, sizeof(char));

	printf("username=");
	scanf("%s", username);
	printf("password=");
	scanf("%s", password);

	JSON_Value *value = json_value_init_object();
	JSON_Object *object = json_value_get_object(value);
	json_object_set_string(object, "username", username);
	json_object_set_string(object, "password", password);
	return json_serialize_to_string_pretty(value);
}

int main(int argc, char *argv[]) {
	int connected = 0, library = 0;
	char host[16] = "34.118.48.238";
	int port = 8080;
	int socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

	char *user[1], *cookies[1], cookie[100], token[BUFLEN], *addbook[1];
	char comanda[100];
	while(1) {
		fgets(comanda, 100, stdin);

		if(strncmp(comanda, "exit", 4) == 0) {
			// iesim din while
			break;
		} else {
			// pentru orice comanda se va deschide o conexiune cu serverul
			socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

			if (strncmp(comanda, "register", 8) == 0) {
				user[0] = user();
				// verificam daca utilizatorul a mai fost inregistrat
				if(strstr(rcv_post_request(socket, host, REGISTER, user, NULL), "is taken") != NULL) {
					printf("TAKEN USERNAME! Please try another one.\n");
				} else {
					printf("DONE! You are now registred.\n");
				}

			}

			else if (strncmp(comanda, "login", 5) == 0) {
				connected = 1;
				user[0] = user();
				char *cookie_ptr = strstr(rcv_post_request(socket, host, LOGIN, user, NULL), "Set-Cookie: ");

				// retinem cookie-ul in functie de care ne dam seama daca un utilizator
				// este conectat sau nu
				if (cookie_ptr == NULL) {
					printf("FAILED!\n");
					connected = 0;
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
				// accesul se face doar daca suntem logati
				if(connected) {
					library = 1;
					// retinem token-ul care ne va ajuta sa verificam accesul catre librarie
					char *tok = strstr(rcv_get_request(socket, host, ACCESS, token, cookies, "get"), "token");
					if (tok == NULL) {
						printf("NO ACCESS!\n");
						library = 0;
					} else {
						tok += 8;
						memset(token, 0, BUFLEN);
						strcpy(token, tok);
						token[strlen(token) - 2] = '\0';
						printf("Success!\n");
					}
				} else {
					printf("Please log in.\n");
				}
			}

			else if (strncmp(comanda, "get_books", 9) == 0) {
				if (library == 1) {
					// accesul se realizeaza doar daca suntem intrati in librarie
					printf("%s\n", strstr(rcv_get_request(socket, host, BOOKS, token, cookies, "get"), "["));
				} else {
					printf("Please enter the library!\n");
				}
			}
			else if (strncmp(comanda, "get_book", 8) == 0) {
				if (library == 1) {
					// accesul se realizeaza doar daca suntem intrati in librarie
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

					// cautam cartile retinute
					if (strstr(rcv_get_request(socket, host, route, token, cookies, "get"), "No book was found!") != NULL) {
						printf("NO BOOK! Entered id is not valid!\n");
					} else {
						printf("%s\n", strstr(rcv_get_request(socket, host, route, token, cookies, "get"), "["));
					}

				} else {
					printf("Please enter the library!\n");
				}
			}

			else if (strncmp(comanda, "add_book", 8) == 0) {
				if (library == 1) {
					// accesul se realizeaza doar daca suntem intrati in librarie
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
					addbook[0] = json_serialize_to_string_pretty(value);

					rcv_post_request(socket, host, BOOKS, addbook, token);
					printf("Success!\n");
				} else {
					printf("Please enter the library!\n");
				}
			}
			else if (strncmp(comanda, "delete_book", 11) == 0) {
				if (library == 1) {
					// accesul se realizeaza doar daca suntem intrati in librarie

					char route[BUFLEN];
					int book_id = 0;

					printf("id=");
					scanf("%d", &book_id);
					if(book_id < 0) {
						printf("WRONG FORMAT! Please try again.\n");
						scanf("%d", &book_id);
					}

					sprintf(route, "%s/%d", BOOKS, book_id);

					// verificam daca id-ul corespunde unei carti
					char *delete = strstr(rcv_get_request(socket, host, route, token, cookies, "delete"), "No book was deleted!");
					if (delete != NULL) {
						printf("NO BOOK! Entered id is not valid!\n");
					}
					printf("Success!\n");
				} else {
					printf("Please enter the library!\n");
				}
			}

			else if (strncmp(comanda, "logout", 6) == 0) {
				if (connected == 1) {
					// comanda se realizeaza doar daca suntem logati deja
					rcv_get_request(socket, host, LOGOUT, token, cookies, "get");
					connected = 0;
					library = 0;
					printf("Success!\n");
				} else {
					printf("Not logged in!\n");
				}
			}
			// inchidem conexiunea deschisa
			close_connection(socket);
		}
	}
}