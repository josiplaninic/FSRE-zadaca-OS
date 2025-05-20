#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// ▪ S pomoću više dretvi riješiti problem pet filozofa koristeći
// koncept monitora. Pri svakoj promjeni program mora
// vizualno prikazati za sve filozofe što oni rade. Npr. kada
// filozof 4 ide jesti, tada treba ispis izgledati otprilike ovako:
// "Stanje filozofa: X o O X o" (X-jede, O-razmišlja, o-čeka na
// štapiće).
// ▪ Problem pet filozofa. Filozofi obavljaju samo dvije različite
// aktivnosti: misle ili jedu. To rade na poseban način. Na
// jednom okruglom stolu nalazi se pet tanjura te pet štapića
// (između svaka dva tanjura po jedan). Filozof prilazi stolu,
// uzima lijevi štapić, pa desni te jede. Zatim vraća štapiće na
// stol i odlazi misliti.

#define BROJ_FILZOFA 5

pthread_mutex_t monitor;
pthread_cond_t uvjet[BROJ_FILZOFA];
int stapic[BROJ_FILZOFA] = {1, 1, 1, 1, 1};
char filozof[BROJ_FILZOFA] = {'O', 'O', 'O', 'O', 'O'}; 

//prikaz stanja
void ispis_stanja() {
    printf("\n\033[1;34m┌────────────────────────────────────────────┐\033[0m\n");
    printf("\033[1;34m│ Stanje filozofa:                          │\033[0m\n");
    printf("\033[1;34m│ ");
    for (int i = 0; i < BROJ_FILZOFA; i++) {
        printf("[%d:%c] ", i, filozof[i]);
    }
    printf("                │\033[0m\n");

    printf("\033[1;34m│ Stanje štapića:                           │\033[0m\n");
    printf("\033[1;34m│ ");
    for (int i = 0; i < BROJ_FILZOFA; i++) {
        printf("%d:%s ", i, stapic[i] ? "🟢" : "🔴");
    }
    printf("               │\033[0m\n");

    printf("\033[1;34m└────────────────────────────────────────────┘\033[0m\n\n");
    fflush(stdout);
}

void misliti(int n) {
    filozof[n] = 'O';
    pthread_mutex_lock(&monitor);
    ispis_stanja();
    pthread_mutex_unlock(&monitor);
    sleep(5);
}

void jesti(int n) {
    pthread_mutex_lock(&monitor);

    filozof[n] = 'o'; // čeka na štapiće
    ispis_stanja();

    while (stapic[n] == 0 || stapic[(n + 1) % BROJ_FILZOFA] == 0) {
        printf("⚠️  Filozof %d čeka jer su štapići zauzeti.\n", n);
        fflush(stdout);
        pthread_cond_wait(&uvjet[n], &monitor);
    }

    stapic[n] = stapic[(n + 1) % BROJ_FILZOFA] = 0; // uzima štapiće
    filozof[n] = 'X'; // jede
    ispis_stanja();

    pthread_mutex_unlock(&monitor);
    sleep(5);

    pthread_mutex_lock(&monitor);
    filozof[n] = 'O'; // ponovno razmišlja
    stapic[n] = stapic[(n + 1) % BROJ_FILZOFA] = 1;

    pthread_cond_signal(&uvjet[(n - 1 + BROJ_FILZOFA) % BROJ_FILZOFA]); 
    pthread_cond_signal(&uvjet[(n + 1) % BROJ_FILZOFA]);

    ispis_stanja();
    pthread_mutex_unlock(&monitor);
}

void *filozof_dretva(void *arg) {
    int n = *(int *)arg;
    while (1) {
        misliti(n);
        jesti(n);
    }
    return NULL;
}

int main() {
    pthread_t fil[BROJ_FILZOFA];
    int id[BROJ_FILZOFA];

    pthread_mutex_init(&monitor, NULL);
    for (int i = 0; i < BROJ_FILZOFA; i++) {
        pthread_cond_init(&uvjet[i], NULL);
    }

    for (int i = 0; i < BROJ_FILZOFA; i++) {
        id[i] = i;
        pthread_create(&fil[i], NULL, filozof_dretva, &id[i]);
    }

    for (int i = 0; i < BROJ_FILZOFA; i++) {
        pthread_join(fil[i], NULL);
    }

    pthread_mutex_destroy(&monitor);
    for (int i = 0; i < BROJ_FILZOFA; i++) {
        pthread_cond_destroy(&uvjet[i]);
    }

    return 0;
}
