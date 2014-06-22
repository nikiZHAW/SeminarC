/*
 * File:		server.c
 * Author:		Niki Hausammann
 * Modul:		Seminar Concurrent Programming
 *
 * Created:		23.03.2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
//#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/sem.h>
#include <signal.h>

//#include "mylib.h"

#define MAX_PEND 5 /* Maximale Anzahl ausstehender Verbindungen */
#define MAX_FILES 50 /* Maximale Anzahl Dateien */
#define MAX_FILENAME 255 /* Maximale Länge Dateiname */
#define MAX_FILESIZE 2048 /* Maximale Dateigrösse (Inhalt) */
#define REF_FILE "./shm_ref" /* Referenz-Datei für Shared Memory der Dateiverwaltung */

extern int errno;

// Struct für Filestruktur / Informationen
struct fileheader {
	char filename[MAX_FILENAME];
	int filelength;
	//int shm_key;
	//int sem_id;
	char content[MAX_FILESIZE];
};

struct fileheader *fh;
key_t shm_fh_key, sem_fh_key;
int sem_fh_id;
int shm_fh_id;
int server_socket, client_socket;

union semun {
	int val;
	struct semid_ds *buf;
	ushort * array;
};

union semun semun_lock, semun_unlock;

enum exit_type { PROCESS_EXIT, /*THREAD_EXIT,*/ NO_EXIT };

void cleanup();

void handle_error(int retcode, const char *msg, enum exit_type et);

void cleanup() {
	int retcode;


/*	for (int i = 0; i < MAX_FILES; i++){
		shmctl(fh[i].shm_key, IPC_RMID, NULL);
	}*/

	retcode = shmctl(shm_fh_id, IPC_RMID, NULL);
	handle_error(retcode, "Entfernen von SHM fehlgeschlagen", PROCESS_EXIT);

	retcode = semctl(sem_fh_id, 0, IPC_RMID, NULL);
	close(server_socket);
	close(client_socket);
}

void handle_error(int retcode, const char *msg, enum exit_type et) {
	if (retcode < 0){
		/* printf("*** Fehler Aufgetreten:\n");
		printf("***   - Fehlernummer: %d %s\n", errno, strerror(errno));
		printf("***   - Rückgabewert: %d\n", retcode);
		printf("***   - Fehlermeldung: %s\n", msg);

		perror("Fehler aufgetreten");*/

		switch (et) {
		case PROCESS_EXIT:
			perror("Beende Prozess wegen Fehler");
			//printf("*** Beende Programm wegen Fehler\n");
			cleanup();
			exit(0);
			break;
		/*case THREAD_EXIT:
			pthread_exit();
			break;*/
		case NO_EXIT:
			perror("Fehler, führe Programm weiter aus");
			//printf("*** Führe Programm weiter aus\n");
			break;
		}
	}
};

int create_shm(key_t key, int size, const char *txt, const char *etxt) {
	int shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
	//handle_error(shm_id, etxt, PROCESS_EXIT);
	return shm_id;
}

int create_sem(key_t key, int size, const char *txt, const char *etxt) {
	int retcode;

	retcode = semget(key, size, IPC_CREAT | 0600);
	handle_error(retcode, "Fehler bei Semaphor-Erstellung", PROCESS_EXIT);
	return retcode;
}

void sighandler(int sig) {
	cleanup();
	exit(0);
}

void display_help(int help_id){
	printf("Hilfe für");
}

void usage() {
	printf("*************************\n");
	printf("Fileserver Niki Hausammann\n");
	printf("Verwendung: server <Port>\n");
	printf("Beispiel: server 5555\n\n");
	exit(1);
}

// Handling der Client-Befehle
void handle_request(int client_socket){
	//printf("handle client\n"); zu Debug-Zwecken

	int retcode;

	semun_lock.val = 0;
	semun_unlock.val = 1;
	//union semun semun_temp;

	struct sembuf sem_buf_lock, sem_buf_unlock, sem_buf_temp;

	sem_buf_lock.sem_num = 0;
	sem_buf_lock.sem_op = -1;
	sem_buf_lock.sem_flg = SEM_UNDO;

	sem_buf_unlock.sem_num = 0;
	sem_buf_unlock.sem_op = 1;
	sem_buf_unlock.sem_flg = SEM_UNDO;

	sem_buf_temp = sem_buf_lock;



	/* Filehandling Part */
	while (1){

		//printf("--- Back at start from while-part ---\n"); zu Debug-Zwecken

		char trennzeichen[] = " ,;:\n";

		char buffer[4096];

		memset(buffer, '\0', 4096);

		retcode = read(client_socket,buffer,255);
		handle_error(retcode, "Lesen von Socket fehlgeschlagen", PROCESS_EXIT);

		char *command = strtok(buffer,trennzeichen);
		char *filename = strtok(NULL, trennzeichen);
		char *length = strtok(NULL, trennzeichen);

				/* Befehlverarbeitung LIST */
		if (strcmp(command, "LIST") == 0){

			//printf("command: %s\nfilename: %s\nlength: %s\n", command, filename, length); zu Debug-Zwecken



			char filenames[257*MAX_FILES];
			memset(filenames,'\0',sizeof(filenames));
			char msgl[(MAX_FILES+1)*256];
			memset(msgl, '\0',sizeof(msgl));


			sem_buf_temp = sem_buf_lock;
			sem_buf_temp.sem_num = MAX_FILES;

			retcode = semop(sem_fh_id, &sem_buf_temp,1);
			handle_error(retcode, "semop() lock in LIST", NO_EXIT);



			int filecount;
			filecount = 0;
			char temparr[100][255];
			int n;
			n = 0;

			for (n = 0; n < MAX_FILES; n++){

				if (strcmp(fh[n].filename, "") != 0){

					strcpy(temparr[filecount],fh[n].filename);
					//strcat(filenames, fh[n].filename);
					//strcat(filenames, "\n");
					filecount++;
				}
			}

			int m;
			m = 0;
			for (m=0;m<filecount;m++){

				strcat(filenames, temparr[m]);
				strcat(filenames, "\n");
			}

			//printf("filecount: %d\n", filecount);

			sem_buf_temp = sem_buf_unlock;
			sem_buf_temp.sem_num = MAX_FILES;
			retcode = semop(sem_fh_id, &sem_buf_temp, 1);
			handle_error(retcode,"semop() unlock in LIST",NO_EXIT);



			//char msg[256*(filecount+1)];

			if (filecount == 0){
				sprintf(msgl, "ACK 0");
			}
			else {
				sprintf(msgl, "ACK %d\n%s", filecount, filenames);
			}

			retcode = write(client_socket,msgl, strlen(msgl));
			handle_error(retcode, "write to socket", NO_EXIT);

			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';
		}
		/* Befehlverarbeitung READ */
		else if (strcmp(command, "READ") == 0){

			//printf("in READ\n"); zu Debug-Zwecken
			//printf("command: %s\nfilename: %s\nlength: %s\n", command, filename, length);zu Debug-Zwecken

			int i = 0;
			int notfound = 0;
			char msgr[2300];
			memset(msgr, '\0',sizeof(msgr));

			for (i=0; i< MAX_FILES; i++) {
				if(strcmp(fh[i].filename, filename) == 0){
					notfound = 1;

					//lock
					sem_buf_temp = sem_buf_lock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() lock in READ", NO_EXIT);

					sprintf(msgr,"FILECONTENT %s %d\n%s",fh[i].filename, fh[i].filelength, fh[i].content );

					//unlock
					sem_buf_temp = sem_buf_unlock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() unlock in READ", NO_EXIT);

				}
			}

			if (notfound == 0) {
				sprintf(msgr, "FILENOTFOUND\n");
			}

			retcode = write(client_socket, msgr, sizeof(msgr));
			handle_error(retcode, "write() READ-Funktion", PROCESS_EXIT);

			msgr[0] = '\0';
			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';
		}
		/* Befehlverarbeitung CREATE */
		else if (strcmp(command, "CREATE") == 0){

			//printf("in CREATE\n"); zu Debug-Zwecken
			//printf("command: %s\nfilename: %s\nlength: %s\n", command, filename, length);

			char msgc[32];
			memset(msgc, '\0',sizeof(msgc));

			int emptypos = 0;
			int i = MAX_FILES +1;


			// Prüfen, ob Datei existiert, sowie suche nach einer freien Position im Fileheader-Array
			for (i = 0; i < MAX_FILES; i++) {

				if (strcmp(fh[i].filename,filename) == 0){

					sprintf(msgc, "FILEEXISTS\n");

					i = MAX_FILES;
				}
			}

			for (i = 0; i < MAX_FILES; i++){
				if (((strcmp(fh[i].filename, "") == 0) && (fh[i].filelength == 0))){
					emptypos = i;
					i = MAX_FILES;
				}
			}



			// Aufbereiten der Messages und Schreiben
			if (strcmp(msgc, "FILEEXISTS\n") == 0){
				//msg = "FILEEXISTS\n";
				//sprintf(msg, "FILEEXISTS\n");
			} else if ((emptypos == MAX_FILES +1) && (i == MAX_FILES)) {
				//msg = "NOSPACE\n";
				sprintf(msgc, "NOSPACE\n");
			} else {
				// Anlegen des Files
				sem_buf_temp = sem_buf_lock;
				sem_buf_temp.sem_num = emptypos;

				semop(sem_fh_id, &sem_buf_temp,1);
				handle_error(retcode, "semop() lock in CREATE", NO_EXIT);

				strcpy(fh[emptypos].filename, filename);
				fh[emptypos].filelength = atoi(length);

				retcode = write(client_socket, "CONTENT", 7);
				retcode = read(client_socket, fh[emptypos].content,fh[emptypos].filelength);

//				fh[emptypos].shm_key = create_shm(IPC_PRIVATE,fh[emptypos].filelength, "create shm file", "create shm for file failed");
//
//				char *filecontent;
//				filecontent = (char *) shmat(fh[emptypos].shm_key, NULL,0);
//
//				//handle_error(retcode, "error schmmmmmmm", PROCESS_EXIT);
//
//				retcode = read(client_socket, filecontent, fh[emptypos].filelength);
//								//strcpy(filecontent,tmpchar);
//				handle_error(retcode, "Content in SHM", PROCESS_EXIT);
//
//				retcode = shmdt(filecontent);

				sem_buf_temp = sem_buf_unlock;
				sem_buf_temp.sem_num = emptypos;

				semop(sem_fh_id, &sem_buf_temp, 1);
				handle_error(retcode, "semop() unlock in CREATE", NO_EXIT);

				sprintf(msgc, "FILECREATED\n");
			}

			retcode = write(client_socket, msgc,strlen(msgc));
			handle_error(retcode, "write() fehlgeschlagen", PROCESS_EXIT);

			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';
		}
		/* Befehlverarbeitung UPDATE */
		else if (strcmp(command, "UPDATE") == 0){

			//printf("in UPDATE\n"); zu Debug-Zwecken
			//printf("command: %s\nfilename: %s\nlength: %s\n", command, filename, length);

			char msgu[32];
			memset(msgu, '\0', sizeof(msgu));

			int i;
			i = 0;
			int notfoundu = 1;
			for (i=0; i< MAX_FILES; i++) {
				if(strcmp(fh[i].filename, filename) == 0){
					notfoundu = 0;

					//lock
					sem_buf_temp = sem_buf_lock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() lock in READ", NO_EXIT);

					memset(fh[i].filename, '\0', sizeof(fh[i].filename));
					memset(fh[i].content, '\0', sizeof(fh[i].content));
					fh[i].filelength = 0;

					strcpy(fh[i].filename, filename);
					fh[i].filelength = atoi(length);

					retcode = write(client_socket, "CONTENT", 7);
					handle_error(retcode,"write() Update", PROCESS_EXIT);
					retcode = read(client_socket, fh[i].content,fh[i].filelength);
					handle_error(retcode,"read() Update", PROCESS_EXIT);

					//unlock
					sem_buf_temp = sem_buf_unlock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() unlock in READ", NO_EXIT);

					sprintf(msgu,"UPDATED\n");

				}
			}

			if (notfoundu == 1) {
				sprintf(msgu, "NOSUCHFILE\n");
			}

			printf("status");
			retcode = write(client_socket, msgu, sizeof(msgu));
			handle_error(retcode,"write() Update", PROCESS_EXIT);

			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';

		}
		/* Befehlverarbeitung DELETE */
		else if (strcmp(command, "DELETE") == 0){

			//printf("in DELETE\n"); zu Debug-Zwecken
			//printf("command: %s\nfilename: %s\nlength: %s\n", command, filename, length);

			int i = 0;
			int notfoundd = 1;
			char msgd[32];
			memset(msgd, '\0',sizeof(msgd));

			for (i=0; i< MAX_FILES; i++) {
				if(strcmp(fh[i].filename, filename) == 0){
					notfoundd = 0;

					//lock
					sem_buf_temp = sem_buf_lock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() lock in READ", NO_EXIT);

					memset(fh[i].filename, '\0', sizeof(fh[i].filename));
					memset(fh[i].content, '\0', sizeof(fh[i].content));
					fh[i].filelength = 0;

					//unlock
					sem_buf_temp = sem_buf_unlock;
					sem_buf_temp.sem_num = i;

					retcode = semop(sem_fh_id, &sem_buf_temp,1);
					handle_error(retcode, "semop() unlock in READ", NO_EXIT);

					sprintf(msgd,"DELETED\n");

				}
			}

			if (notfoundd == 1) {
				sprintf(msgd, "NOSUCHFILE\n");
			}

			retcode = write(client_socket, msgd, sizeof(msgd));
			handle_error(retcode,"write() Delete", PROCESS_EXIT);

			msgd[0] = '\0';
			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';
		}
		/* Falscher Befehl */
		else {

			retcode = write(client_socket, "Befehl nicht bekannt\n",21);
			handle_error(retcode,"write() Falscher Befehl", PROCESS_EXIT);

			command ='\0';
			filename ='\0';
			length ='\0';
			buffer[0] ='\0';
		}
	}

}

// Main Function
int main(int argc, char *argv[]) {

	signal(SIGINT, sighandler);
	int retcode;

	// Startparameter überprüfen
	if (argc !=2) {
		usage();
	}

	// Ref-File überprüfen und ggf erstellen
	FILE *ref_file_ptr;
	ref_file_ptr = fopen(REF_FILE, "r+");
	if (ref_file_ptr == NULL){
		ref_file_ptr = fopen(REF_FILE, "w+");

		if (ref_file_ptr == 0){
			perror("Datei konnte nicht erstellt werden");
		}
	}
	retcode = fclose(ref_file_ptr);
	handle_error(retcode,"fclose() Fehlgeschlagen", NO_EXIT);

	// Shared Memory für Kontrollstruktur erstellen
	shm_fh_key = ftok(REF_FILE, 1);
	shm_fh_id = create_shm(shm_fh_key, (MAX_FILES * (MAX_FILESIZE+270)), "create", "create_shm() failed");

	printf("shm_id %d\n", shm_fh_id);

	fh = (struct fileheader *) shmat(shm_fh_id, NULL, 0);

	// Semaphor erstellen, Vorbereitungen

	sem_fh_key = ftok(REF_FILE, 1);
	sem_fh_id = create_sem(sem_fh_key, MAX_FILES+1, "erstelle Semaphor", "Semaphor fehlgeschlagen");

	semun_lock.val = 0;
	semun_unlock.val = 1;

	// Initialisiere die Dateistruktur "Formatieren". MAX_FILES + 1 ist der Semaphor für die Kontrollstruktur
	for (int i = 0; i < (MAX_FILES +1); i++){
		fh[i].filelength = 0;
		fh[i].filename[0] = '\0';
		retcode = semctl(sem_fh_id,i,SETVAL, semun_unlock);
		handle_error(retcode, "init sem", PROCESS_EXIT);
		//fh[i].shm_key = 0; Vorgesehen für Variante 2 gemäss Seminarbericht
		fh[i].content[0] = '\0';
	}

	// Welcome Screen
	printf("*************************\n");
	printf("Multi-User Fileserver\n");
	printf("*************************\n");

	// Server initialisieren
	//int server_socket; /* Socketdeskriptor für Server*/
	//int client_socket; /* Socketdeskriptor für Client*/
	struct sockaddr_in server_addr; /* IP-Adresse des lokalen Servers */
	struct sockaddr_in client_addr;	/* IP-Adresse des Clients */
	unsigned short server_port; /* Port des Servers für IP-Verbindungen */
	unsigned int client_addr_len;

	server_port = atoi(argv[1]);
	//printf("server_port %d\n", server_port);
	server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	handle_error(server_socket,"socket failed", NO_EXIT);
	//printf("serv_sock: %d\n",serv_sock);

	/* Adress-Struktur */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; /* Internet Address Family */
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	server_addr.sin_port = htons(server_port); /* local server port */

	/* binding */
	retcode = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
	handle_error(retcode, "binding failed", NO_EXIT);

	/* make socket listening to incoming connections */
	retcode = listen(server_socket,MAX_PEND);
	handle_error(retcode, "listen failed", NO_EXIT);


	while (true) {
		client_addr_len = sizeof(client_addr);

		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
		handle_error(client_socket, "accept failed", NO_EXIT);

		int pid;

		pid = fork();

			if (pid < 0)
		        {
		            perror("fork() Fehler beim Erstellen von Child-Prozess");
		            exit(1);
		        }

			else if (pid > 0)
		        {
		            continue;
		        }
			else
		        {
		            /* This is the client process */
		            close(server_socket);
		            handle_request(client_socket);
		            exit(0);
		        }
	}

	return 0;
}
