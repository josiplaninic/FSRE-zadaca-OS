#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Modelirati vrtuljak (ringišpil) s dva tipa dretvi/procesa:
// dretvama/procesima posjetitelj (koje predstavljaju posjetitelje koji žele na
// vožnju) te dretvom/procesom vrtuljak. Dretvama/procesima posjetitelj se ne
// smije dozvoliti ukrcati na vrtuljak kada više nema praznih mjesta (kojih je
// ukupno N) te prije nego li svi prethodni posjetitelji siđu. Vrtuljak se može
// pokrenuti tek kada je pun. Za sinkronizaciju koristiti opće semafore i dodatne
// varijable.

#define BROJ_SJEDALA 4
#define BROJ_POSJETITELJA 10

// Semafori
sem_t sem_sva_sjedala_zauzeta;
sem_t sem_voznja_gotova;
sem_t sem_slobodna_sjedala;
sem_t mutex;

// Brojač zauzetih sjedala (zajednički resurs)
int zauzeta_sjedala = 0;


//funkcija vezana uz dretvu posjetitelj
void *posjetitelj(void *arg) {
    long id = (long)arg;

    while (1) {

        // sem_wait dekrementira vrijednost semafora sem_slobodna_sjedala ako je ona veća od 0 (time "zauzima"
        //  jedno slobodno mjesto u vrtuljku) te nastavlja sa svojim radom.
        // Ako je vrijednost semafora 0, dretva se blokira dok neka druga dretva ne pozove sem_post na istom 
        // semaforu (tj. dok netko ne oslobodi mjesto).

        sem_wait(&sem_slobodna_sjedala);

        // mutex je semafor koji ima dozvoljenu vrijednost samo 0 ili 1 (binarni semafor).
        // Koristi se za zaštitu kritičnih odsječaka, tako da u svakom trenutku samo jedna dretva
        // može pristupiti dijeljenom resursu.
        
        sem_wait(&mutex);


        //kriticni odsjecak - samo jedna posjtetiteljska dretva pristupa djeljenom resursu(zauzeta sjedala)
        if (zauzeta_sjedala < BROJ_SJEDALA) {
            zauzeta_sjedala++;
            printf("🧍 Posjetitelj %ld je sjeo (%d/%d).\n", id, zauzeta_sjedala, BROJ_SJEDALA);
            if (zauzeta_sjedala == BROJ_SJEDALA) {
                sem_post(&sem_sva_sjedala_zauzeta); //ukoliko su mjesta popunjena to signaliziramo
            }
        }
        sem_post(&mutex); //izlaz dretve iz kriticnog odsjecka 

        // Čekaj kraj vožnje
        sem_wait(&sem_voznja_gotova);

        // svaki posjetittelj nakon zavrsene voznje ceka ulazak u kriticni odsjecak kako bi pojedinacno sisao sa vrtuljka
        sem_wait(&mutex);
        printf("🚶 Posjetitelj %ld silazi.\n", id);
        zauzeta_sjedala--;

        //ovom provjerom cekamo da svi ustanu sa vrtuljka i tek onda oslobadjamo mjesta
        //bitno je napraviti distinkciju da ako mjesto nije zauzeto ne mora znaciti da je slobodno,
        //cekamo da se svi prvo ustanu i tek onda mogu dolaziti novi posjetitelji

        if (zauzeta_sjedala == 0) {
            for (int i = 0; i < BROJ_SJEDALA; i++) {
                sem_post(&sem_slobodna_sjedala); // novi ciklus
            }
        }
        // dretva izlazi iz kriticnog odsjecka
        sem_post(&mutex);

        sleep(2); 
    }

    return NULL;
}

void *vrtuljak(void *arg) {
    while (1) {
    
        sem_wait(&sem_sva_sjedala_zauzeta); //cekamo da signal bude 1, tj. da su sva sjedala zauzeta
        printf("\n Vrtuljak kreće!\n");
        sleep(3); // simuliramo trajanje vožnje
        printf(" Vrtuljak staje!\n");

        // Dozvoli svima silazak
        for (int i = 0; i < BROJ_SJEDALA; i++) {
            sem_post(&sem_voznja_gotova); 
        }

        sleep(2); // kratka pauza između vožnji
    }

    return NULL;
}

int main() {
    pthread_t vrtuljak_tid, posjetitelj_tids[BROJ_POSJETITELJA];

    // Inicijalizacija semafora
    sem_init(&sem_sva_sjedala_zauzeta, 0, 0);
    sem_init(&sem_voznja_gotova, 0, 0);
    sem_init(&sem_slobodna_sjedala, 0, BROJ_SJEDALA);
    sem_init(&mutex, 0, 1);

    // Pokreni vrtuljak 
    pthread_create(&vrtuljak_tid, NULL, vrtuljak, NULL);

    // Pokreni posjetitelje
    for (long i = 0; i < BROJ_POSJETITELJA; i++) {
        pthread_create(&posjetitelj_tids[i], NULL, posjetitelj, (void *)i);
    }

    // Čekaj sve posjetitelje (neće se dogoditi, ali da se program ne završi)
    for (int i = 0; i < BROJ_POSJETITELJA; i++) {
        pthread_join(posjetitelj_tids[i], NULL);
    }

    sem_destroy(&sem_sva_sjedala_zauzeta);
    sem_destroy(&sem_voznja_gotova);
    sem_destroy(&sem_slobodna_sjedala);
    sem_destroy(&mutex);

    return 0;
}
