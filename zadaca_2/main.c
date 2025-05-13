#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// ZADAĆA 2. 
//  Dekkerov postupak međusobnog isključivanja
//  Ostvariti sustav paralelnih procesa/dretvi. Struktura
// procesa/dretvi dana je sljedećim pseudokodom:
// proces proc(i)
// /* i [0..n-1] */
// za k = 1 do 5 čini
// uđi u kritični odsječak
// za m = 1 do 5 čini
// ispiši (i, k, m)
// izađi iz kritičnog odsječka
// kraj.
//  Međusobno isključivanje ostvariti za dva procesa/dretve
// međusobnim isključivanjem po Dekkerovom algoritmu


#define N 2  // broj procesa (2 za Dekker)

// Zajedničke varijable
int *ZASTAVICA; // predstavlja zastavicu za svaki proces 
int *PRAVO; //predstavlja proces koji ima pravo pristupa kritičnom dijelu

void udji_u_kriticni_odsjecak(int i, int j) {
    ZASTAVICA[i] = 1;
    while (ZASTAVICA[j]) {
        if (*PRAVO == j) {
            ZASTAVICA[i] = 0;
            while (*PRAVO == j) {
                // busy wait
            }
            ZASTAVICA[i] = 1;
        }
    }
}

void izadji_iz_kriticnog_odsjecka(int i, int j) {
    *PRAVO = j;
    ZASTAVICA[i] = 0;
}

void proc(int i) {
    int j = 1 - i;
    for (int k = 1; k <= 5; k++) {
        udji_u_kriticni_odsjecak(i, j);
        for (int m = 1; m <= 5; m++) {
            printf("Proces %d: k = %d, m = %d\n", i, k, m);
            sleep(1);
        }
        izadji_iz_kriticnog_odsjecka(i, j);
    }
    exit(0);
}

int main() {
    int sid_z = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0600);
    int sid_p = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0600);
    if (sid_z == -1 || sid_p == -1) {
        perror("shmget");
        exit(1);
    }

    ZASTAVICA = shmat(sid_z, NULL, 0);
    PRAVO = shmat(sid_p, NULL, 0);

    ZASTAVICA[0] = 0;
    ZASTAVICA[1] = 0;
    *PRAVO = 0;

    pid_t pid1 = fork();
    if (pid1 == 0) proc(0);

    pid_t pid2 = fork();
    if (pid2 == 0) proc(1);

    wait(NULL);
    wait(NULL);

    shmdt(ZASTAVICA);
    shmdt(PRAVO);
    shmctl(sid_z, IPC_RMID, NULL);
    shmctl(sid_p, IPC_RMID, NULL);

    return 0;
}
