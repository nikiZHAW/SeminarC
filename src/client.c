/*
 * File:		client.c
 * Author:		Niki Hausammann
 * Modul:		Seminar Concurrent Programming
 *
 * Created:		23.03.2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 4096

//#include "mylib.h"

int client_socket;

enum exit_type { PROCESS_EXIT, /*THREAD_EXIT,*/ NO_EXIT };

void handle_error(int retcode, const char *msg, enum exit_type et);

void usage();

void sighandler(int sig);

void write_sock(int sock, char *data, int length){
	int retcode = write(sock, data, length);
	handle_error(retcode, "write() Fehler", NO_EXIT);
}

int read_sock(int sock, char *data, int length){
	int retcode = read(sock, data, length);
	handle_error(retcode, "read() Fehler", NO_EXIT);
	return retcode;
}

int main(int argc, char *argv[]) {

	signal(SIGINT, sighandler);
	int retcode;

	struct sockaddr_in server_sockaddr;
	unsigned short server_port;
	char *server_addr;

	if (argc < 3 || argc > 3) {
		usage();
	}

	server_addr = argv[1];
	server_port = atoi(argv[2]);

	client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	handle_error(client_socket, "socket failed", PROCESS_EXIT);

	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = inet_addr(server_addr);
	server_sockaddr.sin_port = htons(server_port);

	retcode = connect(client_socket,(struct sockaddr *) &server_sockaddr,sizeof(server_sockaddr));
	handle_error(retcode,"connect failed",PROCESS_EXIT);

	printf("Mit Server verbunden\n");

	char receive_string[BUF_SIZE];
	char send_string[BUF_SIZE];

	while (1){

		memset(send_string, '\0', BUF_SIZE);
		retcode = fgets(send_string, BUF_SIZE ,stdin);

		write_sock(client_socket, send_string, strlen(send_string));

		memset(receive_string, '\0', BUF_SIZE);
		retcode = read_sock(client_socket, receive_string, BUF_SIZE);

		printf("%s\n", receive_string);

	}
}

void usage() {
	printf("*************************\n");
	printf("Client für Fileserver Niki Hausammann\n");
	printf("Verwendung: client <Server-IP> <Server-Port>\n");
	printf("Beispiel: server 127.0.0.1 5555\n\n");
	exit(1);
}

void sighandler(int sig) {
	close(client_socket);
	exit(0);
}

void handle_error(int retcode, const char *msg, enum exit_type et) {
	if (retcode < 0){
		perror("Fehler aufgetreten");
		switch (et) {
		case PROCESS_EXIT:
			perror("*** Beende Programm wegen Fehler\n");
			exit(0);
			break;
		/*case THREAD_EXIT:
			pthread_exit();
			break;*/
		case NO_EXIT:
			perror("*** Führe Programm weiter aus\n");
			break;
		}
	}
};
