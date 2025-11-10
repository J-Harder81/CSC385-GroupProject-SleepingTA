#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// memset
#include <pthread.h>	// pthread_t, pthread_create, pthread_join
#include <semaphore.h>	// sem_init, sem_wait, sem_post, sem_destroy
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>	// atomic_int

// ---------- Constants ----------
#define NUM_SEAT   3       // Number of hallway chairs available
#define SLEEP_MAX  5       // Max sleep time for TA and students (in seconds)

// Toggle: 1 = student leaves after one help; 0 = student keeps re-queuing
#ifndef ONE_AND_DONE
#define ONE_AND_DONE 1
#endif

// ---------- Global synchronization primitives ----------
sem_t sem_stu;             // Semaphore to wake the TA when students arrive
sem_t *stu_done = NULL;    // Array of semaphores for each student to wait for help completion
pthread_mutex_t mutex;     // Mutex to protect shared state (chair queue, counters)

// ---------- Shared state for waiting area ----------
int chair[NUM_SEAT] = {0}; // Circular queue representing hallway chairs (holds student IDs)
int count = 0;             // Number of students currently waiting
int next_seat = 0;         // Index for next student to sit
int next_teach = 0;        // Index for TA to serve next student

// ---------- Simulation control ----------
int g_student_num = 0;     // Total number of students
atomic_int running = 1;    // Flag to control simulation loop
atomic_int *helped = NULL; // Array to track which students have been helped

// ---------- Function prototypes ----------
void rand_sleep(void);
void* stu_programming(void* stu_id_ptr);
void* ta_teaching(void* unused);

// ---------- Main Program ----------
int main(void) {
    pthread_t *students = NULL;
    pthread_t ta;
    int *student_ids = NULL;

    // Prompt user for number of students
    printf("Please enter the number of students being helped.\n\n");
    if (scanf("%d", &g_student_num) != 1 || g_student_num < 0) {
        fprintf(stderr, "Invalid number.\n\n");
        return 1;
    }
    if (g_student_num == 0) {
        printf("TA is sleeping. There are no students to help.\n\n");
        return 0;
    }

    // Allocate memory for threads, IDs, semaphores, and help tracking
    students = malloc(sizeof(pthread_t) * g_student_num);
    student_ids = malloc(sizeof(int) * g_student_num);
    stu_done = malloc(sizeof(sem_t) * g_student_num);
    helped = calloc(g_student_num, sizeof(atomic_int));
    if (!students || !student_ids || !stu_done || !helped) {
        perror("malloc/calloc");
        return 1;
    }
    memset(student_ids, 0, sizeof(int) * g_student_num);

    // Initialize semaphores and mutex
    if (sem_init(&sem_stu, 0, 0) != 0) { perror("sem_init sem_stu"); return 1; }
    for (int i = 0; i < g_student_num; i++) {
        if (sem_init(&stu_done[i], 0, 0) != 0) { perror("sem_init stu_done[i]"); return 1; }
    }
    if (pthread_mutex_init(&mutex, NULL) != 0) { perror("pthread_mutex_init"); return 1; }

    // Seed random number generator
    srand((unsigned)time(NULL));

    // Create TA thread
    if (pthread_create(&ta, NULL, ta_teaching, NULL) != 0) {
        perror("pthread_create TA");
        return 1;
    }

    // Create student threads
    for (int i = 0; i < g_student_num; i++) {
        student_ids[i] = i + 1; // Student IDs are 1-based
        if (pthread_create(&students[i], NULL, stu_programming, &student_ids[i]) != 0) {
            perror("pthread_create student");
            atomic_store(&running, 0);
            break;
        }
    }

    // Wait until all students have been helped
    int all_helped = 0;
    while (!all_helped) {
        all_helped = 1;
        for (int i = 0; i < g_student_num; i++) {
            if (atomic_load(&helped[i]) == 0) {
                all_helped = 0;
                break;
            }
        }
        usleep(100000); // Sleep briefly to avoid busy waiting
    }

    // Signal threads to shut down
    atomic_store(&running, 0);
    sem_post(&sem_stu); // Wake TA if sleeping
    for (int i = 0; i < g_student_num; i++) sem_post(&stu_done[i]); // Wake any blocked students

    // Join all threads
    for (int i = 0; i < g_student_num; i++) {
        pthread_join(students[i], NULL);
    }
    pthread_join(ta, NULL);

    // Print help summary
    printf("Help Summary:\n");
    for (int i = 0; i < g_student_num; i++) {
        printf("Student %d: %s\n", i+1, atomic_load(&helped[i]) ? "Helped" : "Not Helped");
    }

    // Cleanup resources
    sem_destroy(&sem_stu);
    for (int i = 0; i < g_student_num; i++) sem_destroy(&stu_done[i]);
    pthread_mutex_destroy(&mutex);
    free(students);
    free(student_ids);
    free(stu_done);
    free(helped);

    printf("Simulation ended.\n");
    return 0;
}

// ---------- Student thread function ----------
void* stu_programming(void* stu_id_ptr) {
    int id = *(int*)stu_id_ptr;
    int my_index = id - 1;

    printf("\t Student %d is on the way to TA office\n\n", id);

    while (atomic_load(&running)) {
        rand_sleep(); // Simulate programming time

        pthread_mutex_lock(&mutex);
        if (count < NUM_SEAT) {
            // Sit in hallway chair
            chair[next_seat] = id;
            count++;
            printf("\t Student %d is waiting seated at hallway\n\n", id);
            printf("\t Student(s) waiting in the chairs : [1] %d [2] %d [3] %d\n\n",
                   chair[0], chair[1], chair[2]);
            next_seat = (next_seat + 1) % NUM_SEAT;
            pthread_mutex_unlock(&mutex);

            sem_post(&sem_stu); // Wake TA
            if (!atomic_load(&running)) break;
            sem_wait(&stu_done[my_index]); // Wait for help

#if ONE_AND_DONE
            break; // Exit after one help session
#else
            continue; // Re-enter programming loop
#endif
        } else {
            pthread_mutex_unlock(&mutex);
            printf("\t Student %d arrived but all waiting chairs are full. Resuming programming and will return later.\n\n", id);
        }
    }
    return NULL;
}

// ---------- TA thread function ----------
void* ta_teaching(void* unused) {
    (void)unused;

    while (atomic_load(&running) || count > 0) {
        sem_wait(&sem_stu); // Wait for student arrival

        pthread_mutex_lock(&mutex);
        if (count == 0) {
            if (!atomic_load(&running)) {
            printf("\t All students have been helped. TA is going back to sleep.\n\n");
        }
        pthread_mutex_unlock(&mutex);
        continue; // No students to help
    }

        // Help next student in FIFO order
        int sid = chair[next_teach];
        printf("\t\t TA is helping Student %d\n\n", sid);

        chair[next_teach] = 0;
        count--;
        printf("\t Student(s) waiting in the chair : [1] %d [2] %d [3] %d\n\n",
               chair[0], chair[1], chair[2]);
        next_teach = (next_teach + 1) % NUM_SEAT;
        pthread_mutex_unlock(&mutex);

        rand_sleep(); // Simulate help session
        printf("\t\t Session finished with Student %d.\n\n", sid);

        // Mark student as helped and signal completion
        int idx = sid - 1;
        if (idx >= 0 && idx < g_student_num) {
            atomic_store(&helped[idx], 1);
            sem_post(&stu_done[idx]);
        }
    }
    return NULL;
}

// Utility function to simulate random sleep
void rand_sleep(void) {
    int t = rand() % SLEEP_MAX + 1;
    sleep((unsigned)t);
}
