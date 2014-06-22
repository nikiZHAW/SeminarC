/*
 * File:		mylib.h
 * Author:		Niki Hausammann
 * Modul:		Seminar Concurrent Programming
 *
 * Created:		23.03.2014
 */

#include <sys/ipc.h>
#include <sys/shm.h>

enum exit_type { PROCESS_EXIT, /*THREAD_EXIT,*/ NO_EXIT };

void handle_error(int retcode, const char *msg, enum exit_type et); /* Behandelt Fehler */

int create_shm(key_t key, int size, const char *txt, const char *etxt); /*Shared Memory erstellen */

void cleanup(int shmid); /* bereinigt Shared Memory */

void sighandler(int sig);

void handle_error(int retcode, const char *msg, enum exit_type et) {
	if (retcode < 0){
		printf("*** Fehler Aufgetreten:\n");
		printf("***   - Fehlernummer: %d\n", errno);
		printf("***   - Fehlermeldung: %s\n", msg);
		switch (et) {
		case PROCESS_EXIT:
			printf("*** Beende Programm wegen Fehler\n");
			exit(0);
			break;
		/*case THREAD_EXIT:
			pthread_exit();
			break;*/
		case NO_EXIT:
			printf("*** Führe Programm weiter aus\n");
			break;
		}
	}
};

int create_shm(key_t key, int size, const char *txt, const char *etxt) {
	int shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
	//handle_error(shm_id, etxt, PROCESS_EXIT);
	return shm_id;
}

void sighandler(int sig) {
	cleanup(shmid)
}

void cleanup(int shmid) {
	int retcode;
	retcode = shmctl(shmid, IPC_RMID, NULL);
	handle_error(retcode, "removing of shm failed", NO_EXIT);
}
