#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define MALLOC_CHECK(ptr) \
    do { \
        if (!(ptr)) { \
            fprintf(stderr, "Fatal: Memory allocation failed at %s:%d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define QUEUESIZE 10
#define LOOP 1
#define P 2
#define C 1

void *producer(void *thread_data);
void *consumer(void *thread_data);
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
    queue *fifo = queueInit();
    pthread_t pro[P], con[C];

    for (int i=0; i<P; i++) {
        struct thread_data *td = malloc(sizeof(struct thread_data));
        MALLOC_CHECK(td);

        td->fifo = fifo;
        td->thread_id = i;

        pthread_create(&pro[i], NULL, producer, (void *)td);
    }

    for (int i=0; i<C; i++){
        struct thread_data *td = malloc(sizeof(struct thread_data));
        MALLOC_CHECK(td);

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

void *producer(void *thread_data){
    queue *fifo = ((struct thread_data *)thread_data)->fifo;
    int thread_id = ((struct thread_data *)thread_data)->thread_id;
    free(thread_data); // Αποδέσμευση μνήμης που δέσμευσε ο main για τα δεδομένα του thread
    struct workFunction task;
    task.work = calculate_sine;

    for (int i=0; i<LOOP; i++){
        double *arg = malloc(sizeof(double));
        *arg = (double)(thread_id * LOOP + i); // Υπολογισμός του επιχειρήματος για την εργασία
        task.arg = arg;
        
        pthread_mutex_lock(fifo->mut);
        while(fifo->full){
            printf("producer %d: queue FULL.\n", thread_id);
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }
    return NULL;
}

void *consumer(void *thread_data){
    queue *fifo = ((struct thread_data *)thread_data)->fifo;
    free(thread_data); // Αποδέσμευση μνήμης που δέσμευσε ο main για τα δεδομένα του thread
    struct workFunction task;

    while (1){
        pthread_mutex_lock(fifo->mut);
        while(fifo->empty){
            printf("consumer: queue EMPTY.\n");
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        task.work(task.arg); // Εκτέλεση της συνάρτησης εργασίας
    }
    return NULL;
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

    return;
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

    return;
}

void *calculate_sine(void *arg){
    double *angle = (double *)arg;
    for (int i = 0; i < 10; i++){
        printf("Angle %.2f: Sine = %.4f\n", *angle + i, sin(*angle + i));
    }
    free(arg); // Αποδέσμευση μνήμης που δέσμευσε ο producer για το επιχείρημα
    return NULL;
}

queue *queueInit(void){
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    MALLOC_CHECK(q);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    MALLOC_CHECK(q->mut);
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    MALLOC_CHECK(q->notFull);
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);   

    return q;
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
