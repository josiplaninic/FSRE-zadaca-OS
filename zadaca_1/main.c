#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

// Neka program simulira neki dugotrajni posao (slično servisima) koji koristi dvije datoteke:
// u jednu dodaje do sada izračunate vrijednosti (npr. kvadrati slijednih brojeva), a u drugu
// podatak do kuda je stigao. Npr. u obrada.txt zapisuje 1 4 9 … (svaki broj u novi red) a u
// status.txt informaciju o tome gdje je stao ili kako nastaviti. Npr. ako je zadnji broj u
// obrada.txt 100 u status.txt treba zapisati 10 tako da u idućem pokretanju može nastaviti
// raditi i dodavati brojeve.
// Prije pokretanja te je datoteke potrebno ručno napraviti i inicijalizirati. Početne vrijednosti
// mogu biti iste – broj 1 u obje datoteke.
// Pri pokretanju programa on bi trebao otvoriti obje datoteke, iz status.txt, pročitati tamo
// zapisanu vrijednost. Ako je ona veća od nule program nastavlja s proračunom s prvom
// idućom vrijednošću i izračunate vrijednosti nadodaje u obrada.txt. Prije nastavka rada u
// status.txt upisuje 0 umjesto prijašnjeg broja, što treba označavati da je obrada u tijeku, da
// program radi.
// Na signal (npr. SIGUSR1) program treba ispisati trenutni broj koji koristi u obradi. Na
// signal SIGTERM otvoriti status.txt i tamo zapisati zadnji broj (umjesto nule koja je tamo)
// te završiti s radom.
// Na SIGINT samo prekinuti s radom, čime će u status.txt ostati nula (čak se taj signal niti
// ne mora maskirati – prekid je pretpostavljeno ponašanje!). To će uzrokovati da iduće
// pokretanje detektira prekid – nula u status.txt, te će za nastavak rada, tj. Određivanje
// idućeg broja morati „analizirati“ datoteku obrada.txt i od tamo zaključiti kako nastaviti
// (pročitati zadnji broj i izračunati korijen). Operacije s datotekama, radi jednostavnosti,
// uvijek mogu biti u nizu open+fscanf/fprintf+close, tj. ne držati datoteke otvorenima da se
// izbjegnu neki drugi problemi. Ali ne mora se tako. U obradu dodati odgodu (npr. sleep(5))
// da se uspori izvođenje.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

static volatile sig_atomic_t CUR        = 1;  /* trenutačni broj */
static volatile sig_atomic_t WANT_USR1  = 0;
static volatile sig_atomic_t WANT_TERM  = 0;

/* ────────── handleri signala ────────── */
static void on_usr1(int s) { (void)s; WANT_USR1 = 1; }
static void on_term(int s) { (void)s; WANT_TERM = 1; }

/* ────────── pomoćne funkcije ────────── */
static long read_status(void)
{
    FILE *f = fopen("status.txt", "r");
    if (!f) return -1;
    long v = 0;
    fscanf(f, "%ld", &v);
    fclose(f);
    return v;
}

static void write_status(long v)
{
    FILE *f = fopen("status.txt", "w");
    if (f) { fprintf(f, "%ld\n", v); fclose(f); }
}

static long read_last_square(void)
{
    FILE *f = fopen("obrada.txt", "r");
    if (!f) return 0;
    long x, last = 0;
    while (fscanf(f, "%ld", &x) == 1) last = x;
    fclose(f);
    return last;
}

/* ────────────────────────────────────── */
int main(void)
{
    /* inicijaliziraj datoteke ako ne postoje */
    if (access("status.txt", F_OK) != 0) {
        write_status(1);
        FILE *o = fopen("obrada.txt", "w");
        fprintf(o, "1\n");
        fclose(o);
    }

    /* utvrdi s kojeg broja krenuti */
    long st = read_status();
    if (st > 0)
        CUR = st;
    else {
        long sq = read_last_square();
        CUR = (long)roundl(sqrtl((long double)sq)) + 1;   
    }

    write_status(0);               /* 0 = obrada je u tijeku */

    signal(SIGUSR1, on_usr1);
    signal(SIGTERM, on_term);      /* SIGINT ostaje default */

    for (;;)
    {
        if (WANT_USR1) {
            printf("Trenutni broj: %d\n", CUR);      /* CUR je sig_atomic_t => %d */
            fflush(stdout);
            WANT_USR1 = 0;
        }

        if (WANT_TERM) {                           /* uredan izlaz */
            write_status(CUR);
            exit(0);
        }

        FILE *o = fopen("obrada.txt", "a");        /* dodaj kvadrat */
        fprintf(o, "%ld\n", (long)CUR * (long)CUR);
        fclose(o);

        sleep(5);                                  /* simuliraj dug rad */
        CUR++;
    }
}
