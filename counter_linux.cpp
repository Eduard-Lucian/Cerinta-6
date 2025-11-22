#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>

// Configurare
const char* SHM_NAME = "/shm_counter_final";
const char* SEM_NAME = "/sem_counter_final";
const int TARGET = 1000;

int main() {
    // Initializare seed random diferit pentru fiecare proces
    srand(time(NULL) ^ getpid());

    // 1. Memorie Partajata
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    ftruncate(shm_fd, sizeof(int));
    int* shared_counter = (int*)mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_counter == MAP_FAILED) { perror("mmap"); return 1; }

    // 2. Semafor (Valoare initiala 1 = liber)
    sem_t* mutex = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED) { perror("sem_open"); return 1; }

    printf("Linux Process [PID %d] started.\n", getpid());

    while (true) {
        // Asteptam accesul (P / Wait)
        if (sem_wait(mutex) < 0) break;



        // "citesc memoria" - verificam daca am terminat
        if (*shared_counter >= TARGET) {
            sem_post(mutex);
            break;
        }

        // "dau cu banul (random(2)) cat timp cade 2 scriu"

        int banul = (rand() % 2) + 1; // Va fi 1 sau 2

        // Logica de bucla: cat timp banul e 2 SI nu am ajuns la 1000
        while (banul == 2 && *shared_counter < TARGET) {
             (*shared_counter)++;
             printf("[PID %d] a dat 2 -> Scriu: %d\n", getpid(), *shared_counter);

             // Daca am ajuns la final, ne oprim
             if (*shared_counter >= TARGET) break;

             // Dau cu banul din nou pentru urmatoarea iteratie din aceeasi sesiune
             banul = (rand() % 2) + 1;
        }

        if (banul == 1) {
            // Doar pentru debug
        }

        // "eliberează semaforul"
        sem_post(mutex);

        usleep(10000);
 }

    printf("Procesul [PID %d] a terminat. Final count: %d\n", getpid(), *shared_counter);

    munmap(shared_counter, sizeof(int));
    close(shm_fd);
    sem_close(mutex);
       return 0;
}
