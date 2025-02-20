#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define THREAD_NUM_PRODUCERS 3  // Número de threads produtoras
#define THREAD_NUM_CONSUMERS 3  // Número de threads consumidoras
#define BUFFER_SIZE 10          // Tamanho máximo da fila de relógios vetoriais

typedef struct {
    int clock[3];  // Vetor de relógio com 3 componentes
} VectorClock;

VectorClock clockQueue[BUFFER_SIZE];
int clockCount = 0;

pthread_mutex_t mutex;
pthread_cond_t condFull;
pthread_cond_t condEmpty;

void produceClock(VectorClock *clock, int id) {
    for (int i = 0; i < 3; i++) {
        clock->clock[i] = rand() % 100;  // Gera valores aleatórios para o relógio vetorial
    }
    printf("(Produtor %d) Produziu relógio: [%d, %d, %d]\n", id, clock->clock[0], clock->clock[1], clock->clock[2]);
}

void consumeClock(VectorClock *clock, int id) {
    printf("(Consumidor %d) Consumiu relógio: [%d, %d, %d]\n", id, clock->clock[0], clock->clock[1], clock->clock[2]);
}

VectorClock getClock() {
    pthread_mutex_lock(&mutex);

    while (clockCount == 0) {
        pthread_cond_wait(&condEmpty, &mutex);
    }

    VectorClock clock = clockQueue[0];
    for (int i = 0; i < clockCount - 1; i++) {
        clockQueue[i] = clockQueue[i + 1];
    }
    clockCount--;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condFull);
    return clock;
}

void submitClock(VectorClock clock) {
    pthread_mutex_lock(&mutex);

    while (clockCount == BUFFER_SIZE) {
        pthread_cond_wait(&condFull, &mutex);
    }

    clockQueue[clockCount] = clock;
    clockCount++;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condEmpty);
}

void *producer(void *args) {
    long id = (long)args;
    while (1) {
        VectorClock clock;
        produceClock(&clock, id);
        submitClock(clock);
        sleep(1);  // Produz um relógio a cada 1 segundo
    }
    return NULL;
}

void *consumer(void *args) {
    long id = (long)args;
    while (1) {
        VectorClock clock = getClock();
        consumeClock(&clock, id);
        sleep(2);  // Consome um relógio a cada 2 segundos
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condEmpty, NULL);
    pthread_cond_init(&condFull, NULL);

    pthread_t producers[THREAD_NUM_PRODUCERS];
    pthread_t consumers[THREAD_NUM_CONSUMERS];

    srand(time(NULL));

    // Cria threads produtoras
    for (long i = 0; i < THREAD_NUM_PRODUCERS; i++) {
        if (pthread_create(&producers[i], NULL, &producer, (void *)i) != 0) {
            perror("Failed to create producer thread");
        }
    }

    // Cria threads consumidoras
    for (long i = 0; i < THREAD_NUM_CONSUMERS; i++) {
        if (pthread_create(&consumers[i], NULL, &consumer, (void *)i) != 0) {
            perror("Failed to create consumer thread");
        }
    }

    // Aguarda threads produtoras
    for (int i = 0; i < THREAD_NUM_PRODUCERS; i++) {
        if (pthread_join(producers[i], NULL) != 0) {
            perror("Failed to join producer thread");
        }
    }

    // Aguarda threads consumidoras
    for (int i = 0; i < THREAD_NUM_CONSUMERS; i++) {
        if (pthread_join(consumers[i], NULL) != 0) {
            perror("Failed to join consumer thread");
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condEmpty);
    pthread_cond_destroy(&condFull);

    return 0;
}