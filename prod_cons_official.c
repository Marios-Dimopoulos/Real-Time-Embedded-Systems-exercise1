#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 1
#define P 2
#define C 1

void *producer(void *q);
void *consumer(void *q);
void *calculate_sine(void *arg);

struct workFunction{
    void *(*work)(void *);
    void *arg;
};

typedef struct{
    struct workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
}queue;


struct thread_data{
    queue *fifo;
    int thread_id;
};

queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction in);
void queueDel(queue *q, struct workFunction *out);

int main(int argc, char *argv[]){
    queue *fifo;
    fifo = queueInit();
    pthread_t pro[P], con[C];

    for (int i=0; i<P; i++) {
        struct thread_data *td = malloc(sizeof(struct thread_data));
        td->fifo = fifo;
        td->thread_id = i;
        pthread_create(&pro[i], NULL, producer, (void *)td);
    }
    for (int i=0; i<C; i++) {
        struct thread_data *td = malloc(sizeof(struct thread_data));
        td->fifo = fifo;
        td->thread_id = i;
        pthread_create(&con[i], NULL, consumer, (void *)td);
    }
    for (int i=0; i<P; i++) {
        pthread_join(pro[i], NULL);
    }
    for (int i=0; i<C; i++) {
        pthread_join(con[i], NULL);
    }
    queueDelete(fifo);  
    return 0;
}

void *producer(void *q){
    queue *fifo;

    fifo = ((struct thread_data *)q)->fifo;
    int my_id = ((struct thread_data *)q)->thread_id;
    free(q); // Αποδέσμευση μνήμης που δέσμευσε το main για τα δεδομένα του thread

    for (int i=0; i<LOOP; i++){
        struct workFunction wf;
        wf.work = calculate_sine;
        double *angle = malloc(sizeof(double));
        *angle = i*my_id;
        wf.arg = angle;
        pthread_mutex_lock(fifo->mut);
        while(fifo->full){
            printf("producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, wf);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }

    return NULL;
}

void *consumer(void *q){
    queue *fifo;
    int i;

    fifo = ((struct thread_data *)q)->fifo;
    struct workFunction wf;

    while (1) {
        pthread_mutex_lock(fifo->mut);
        while(fifo->empty){
            printf("consumer: queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &wf);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        wf.work(wf.arg); // Εκτέλεση της συνάρτησης εργασίας
    }
    return NULL;
}

void *calculate_sine(void *arg) {
    double *angle = (double *)arg;
    for (int i = 0; i < 10; i++) {
        printf("Angle %.2f: Sine = %.4f\n", *angle + i, sin(*angle + i));
    }
    free(arg); // Αποδέσμευση μνήμης που δέσμευσε ο producer
    return NULL;
}

queue *queueInit(void){
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    if (q == NULL){
        return (NULL);
    }
    
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void queueDelete(queue *q){
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue *q, struct workFunction in){
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE){
        q->tail = 0;    
    }

    if (q->tail == q->head){
        q->full = 1;
    }
    q->empty = 0;
}

void queueDel(queue *q, struct workFunction *out){
    *out = q->buf[q->head];
    q->head++;
    if (q->head == QUEUESIZE){
        q->head = 0;
    }

    if (q->head == q->tail){
        q->empty = 1;
    }
    q->full = 0;
}