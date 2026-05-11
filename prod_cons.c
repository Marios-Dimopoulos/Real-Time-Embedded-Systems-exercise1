#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define MALLOC_CHECK(ptr) \
    do { \
        if (!(ptr)) { \
            fprintf(stderr, "Fatal: Memory allocation failed at %s:%d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define QUEUESIZE 10
#define LOOP 10000 
#define P 1                                                                                                                                    
#define C 9

long long total_wait_time = 0;
long long max_wait_time = -1;
long long min_wait_time = -1;
int tasks_completed = 0;
pthread_mutex_t stats_mut = PTHREAD_MUTEX_INITIALIZER;


long long all_wait_times[P * LOOP]; 

struct workFunction{
    void *(*work)(void *);
    void *arg;
    struct timeval enqueue_time; 
};

typedef struct{
    struct workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

struct thread_data{
    queue *fifo;
    int thread_id;
};

void *producer(void *thread_data);
void *consumer(void *thread_data);
void *calculate_sine(void *arg);
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
        struct workFunction poison_task;
        poison_task.work = NULL; 
        poison_task.arg = NULL;
        
        pthread_mutex_lock(fifo->mut);
        while(fifo->full){
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        queueAdd(fifo, poison_task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }

    for (int i=0; i<C; i++) {
        pthread_join(con[i], NULL); 
    }

    if (tasks_completed > 0) {
        double avg_wait_time = (double)total_wait_time / tasks_completed;
        printf("\n--- Στατιστικά Ουράς Αναμονής ---\n");
        printf("Συνολικά Tasks: %d\n", tasks_completed);
        printf("Ελάχιστος χρόνος αναμονής: %lld μs\n", min_wait_time);
        printf("Μέγιστος χρόνος αναμονής: %lld μs\n", max_wait_time);
        printf("Μέσος χρόνος αναμονής: %.2f μs\n", avg_wait_time);
        
        FILE *fp = fopen("wait_times.csv", "w");
        if (fp == NULL) {
            printf("\nΣφάλμα: Δεν μπόρεσε να δημιουργηθεί το αρχείο wait_times.csv\n");
        } else {
            fprintf(fp, "Task_ID,Wait_Time_us\n");
            for (int i = 0; i < tasks_completed; i++) {
                fprintf(fp, "%d,%lld\n", i, all_wait_times[i]);
            }
            fclose(fp);
            printf("\nΤα δεδομένα γράφτηκαν επιτυχώς στο 'wait_times.csv' για τα διαγράμματα.\n");
        }
    }

    queueDelete(fifo);
    return 0;
}

void *producer(void *thread_data){
    queue *fifo = ((struct thread_data *)thread_data)->fifo;
    int thread_id = ((struct thread_data *)thread_data)->thread_id;
    free(thread_data); 
    struct workFunction task;
    task.work = calculate_sine;

    for (int i=0; i<LOOP; i++){
        double *arg = malloc(sizeof(double));
        *arg = (double)(thread_id * LOOP + i); 
        task.arg = arg;
        
        gettimeofday(&task.enqueue_time, NULL);
        
        pthread_mutex_lock(fifo->mut);
        while(fifo->full){
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
    free(thread_data); 
    struct workFunction task;
    struct timeval dequeue_time;

    while (1){ 
        pthread_mutex_lock(fifo->mut);
        while(fifo->empty){
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        if (task.work == NULL) {
            break; 
        }

        gettimeofday(&dequeue_time, NULL);
        
        long long wait_time = (dequeue_time.tv_sec - task.enqueue_time.tv_sec) * 1000000LL + 
                              (dequeue_time.tv_usec - task.enqueue_time.tv_usec);

        pthread_mutex_lock(&stats_mut);
        
        all_wait_times[tasks_completed] = wait_time; 
        
        total_wait_time += wait_time;
        tasks_completed++;
        if (min_wait_time == -1 || wait_time < min_wait_time) min_wait_time = wait_time;
        if (wait_time > max_wait_time) max_wait_time = wait_time;
        
        pthread_mutex_unlock(&stats_mut);

        task.work(task.arg); 
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

void *calculate_sine(void *arg){
    double *angle = (double *)arg;
    for (int i = 0; i < 10; i++){
        sin(*angle + i); 
    }
    free(arg); 
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