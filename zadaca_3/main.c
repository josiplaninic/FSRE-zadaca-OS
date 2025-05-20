#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

#define MAX 100

/*
	JOSIP PLANINIC 2242/RR

	Zadatak 3. - Ostvariti program koji simulira tijek rezervacije stolova u nekom restoranu. 
    Program na početku treba stvoriti određeni broj dretvi koji se zadaje u 
    naredbenom retku. Svaka dretva/proces nakon isteka jedne sekunde provjerava
     ima li slobodnih stolova te slučajno odabire jedan od njih. Nakon odabira,
      dretva ulazi u kritični odsječak te ponovo provjerava je li odabrani stol 
      slobodan. Ako jest, označava stol zauzetim i izlazi iz kritičnog odsječka.
       U oba slučaja, nakon obavljene operacije, ispisuje se trenutno stanje svih 
       stolova te podatci o obavljenoj rezervaciji. Prilikom ispisa za svaki stol 
       mora biti vidljivo je li slobodan, ili ako nije, broj dretve/procesa koja je 
       taj stol rezervirala. Broj stolova se također zadaje u naredbenom retku. Svaka
        dretva ponavlja isti postupak sve dok više nema slobodnih stolova. Program završava
         kada sve dretve završe.

	Primjer ispisa za 3 dretve i 5 stolova:
		Dretva 1: odabirem stol 2
		Dretva 2: odabirem stol 2
		Dretva 3: odabirem stol 5
		Dretva 2: rezerviram stol 2, stanje:
		-2---
		Dretva 1: neuspjela rezervacija stola 2, stanje:
		-2---
		Dretva 3: rezerviram stol 5, stanje:
		-2--3
		itd.
	Upute:
		- Zaštitu kritičnog odsječka postupka rezervacije stola ostvariti koristeći Lamportov algoritam međusobnog isključivanja.

	Lamportov algoritam:
 - Zajedničke varijable: ULAZ[0..n-1], BROJ[0..n-1], sve početno postavljene u nulu.
	- uđi_u_kritični_odsječak(i)
			ULAZ[i] = 1
			BROJ[i] = max ( BROJ[0], ..., BROJ[n-1] ) + 1
			ULAZ[i] = 0

			za j = 0 do n-1 čini
				dok je ULAZ[j] <> 0 čini
					ništa
				dok je BROJ[j] <> 0 && (BROJ[j] < BROJ[i] || (BROJ[j] == BROJ[i] && j < i)) čini
					ništa

	- izađi_iz_kritičnog_odsječka(i)
			BROJ[i] = 0

*/

//dretve medjusobno dijele memorijski prostor unutar procesa, stoga korisitmo globalne varijable
int *stolovi;                 // 0 ako je stol slobodan, broj dretve ako je zauzet
int *ULAZ, *BROJ;             // Lamportove varijable, ULAZ je zastavica koja oznacava da dretva ceka dodjeljenje rednog broja
                              // BROJ predstavlja redni broj cekanja dretve za izvrsavanje vlastitog posla
int broj_stolova, broj_dretvi;


//ulazni argument je indeks dretve
void udji_u_kriticni_odsjecak(int i) {
    //dretvu najprije dodajemo u red cekanja
    //dok ne izracunamo redni broj, zastavica ulaz je 1

    ULAZ[i] = 1;
    int max = 0;
    for (int j = 0; j < broj_dretvi; j++) {
        if (BROJ[j] > max) max = BROJ[j];
    }
    BROJ[i] = max + 1;
    ULAZ[i] = 0;
    
    //privremeno blokiramo rad prosljedjene dretve(dok ne dodje na red)
    //iteriramo kroz redne brojeve dretvi
    for (int j = 0; j < broj_dretvi; j++) {
        //ukoliko postoji dretva koja je u procesu dodjele broja, ili ima manji redni broj(ili manji redni broj i indeks), 
        //cekamo da ta dretva dobije svoj redni broj ili zavrsi izvodjenje svoga posla

        //proslijedjenu dretvu blokiramo ako je ulaz neke druge dretve 1 kako ne bismo koristili zastarjele vrijednosti
        while (ULAZ[j]);

        //ako je vrijednost BROJ[j], ta dretva ne zeli uci u kriticni odsjecak te ju zanemarujemo
        while (BROJ[j] != 0 &&
              (BROJ[j] < BROJ[i] || (BROJ[j] == BROJ[i] && j < i)));
    }
}

void izadji_iz_kriticnog_odsjecka(int i) {
    BROJ[i] = 0;
}

bool postoji_slobodan_stol() {
    for (int i = 0; i < broj_stolova; i++) {
        if (stolovi[i] == 0) return true;
    }
    return false;
}

void ispisi_stanje() {
    for (int i = 0; i < broj_stolova; i++) {
        if (stolovi[i] == 0) printf("-");
        else printf("%d", stolovi[i]);
    }
    printf("\n");
}

//phtread_create ocekuje void* f(void* arg)
void* rezervacija(void *arg) {
    int id = *(int*)arg;

    //dretve rade sve dok postoji bar 1 slobodan stol
    while (postoji_slobodan_stol()) {
        //pauziramo program radi lakse demonstracije izvedbe
        sleep(1); 

        int izbor = rand() % broj_stolova;
        printf("Dretva %d: odabirem stol %d\n", id+1, izbor+1);

        udji_u_kriticni_odsjecak(id); //dretvi dodjeljujemo redni broj te cekamo dok ne dodje na red

        //nakon sto dodje na red, dretva obavlja posao
        if (stolovi[izbor] == 0) {
            stolovi[izbor] = id + 1;
            printf("Dretva %d: rezerviram stol %d, stanje:\n", id+1, izbor+1);
        } else {
            printf("Dretva %d: neuspjela rezervacija stola %d, stanje:\n", id+1, izbor+1);
        }

        ispisi_stanje();

        //nakon sto obavi posao, dretva privremeno izlazi iz kriticnog odsjecka
        izadji_iz_kriticnog_odsjecka(id);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    //shell naredba imati 3 argumenta, naziv .exe datoteke, broj dretvi, broj stolova 
    if (argc != 3) {
        printf("Upotreba: %s broj_dretvi broj_stolova\n", argv[0]);
        return 1;
    }

    //castanje argumenata glavne funkcije iz char* u int
    broj_dretvi = atoi(argv[1]);
    broj_stolova = atoi(argv[2]);

    //alociramo nizove adekvatne velicine
    stolovi = calloc(broj_stolova, sizeof(int));
    ULAZ = calloc(broj_dretvi, sizeof(int));
    BROJ = calloc(broj_dretvi, sizeof(int));

    //alociramo memoriju za niz dretvi te njihovih identifikatora
    pthread_t *tids = malloc(broj_dretvi * sizeof(pthread_t));
    int *ids = malloc(broj_dretvi * sizeof(int));

    //inicijalizacija/kreiranje dretvi
    for (int i = 0; i < broj_dretvi; i++) {
        ids[i] = i;
        pthread_create(&tids[i], NULL, rezervacija, &ids[i]);
    }

    //blokiramo glavnu dretvu dok sve sporedne dretve ne zavrse sa izvodjenjem funkcija
    for (int i = 0; i < broj_dretvi; i++) {
        pthread_join(tids[i], NULL);
    }

    free(stolovi);
    free(ULAZ);
    free(BROJ);
    free(tids);
    free(ids);
    return 0;
}
