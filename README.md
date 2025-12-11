# CSC385-GroupProject-SleepingTA
This group project was collaborative effort to create a multithreaded C Program simulating the classic Sleeping Teaching Assistant (TA) concurrency problem. It models a TA helping students in a computer lab, using semaphores and mutexes to coordinate access to limited hallway chairs and ensure orderly help sessions.

## Overview
**TA Behavior:** Sleeps until a student arrives. Helps one student at a time in FIFO order.\
**Student Behavior:** Programs independently for a random amount of time, then seeks help. If chairs are full, they return to programming and return later.\
**Chairs:** Limited to NUM_SEAT (default: 3). Students wait here if the TA is busy.\
**Termination:** Simulation ends when all students have been helped once (or repeatedly, depending on the ONE_AND_DONE flag).

## Concurrency Model
| Component           | Mechanism                                |
|---------------------|------------------------------------------|
| TA & Students       | `pthread_t` threads                      |
| Chair Queue         | Circular buffer with mutex protection    |
| Wake-up Signals     | `sem_t sem_stu` (students notify TA)     |
| Help Completion     | `sem_t stu_done[i]` (TA notifies student)|
| Shared State        | Protected by `pthread_mutex_t mutex`     |
| Simulation Control  | `atomic_int running`, `atomic_int helped[]` |


## Compilation & Execution
**Compile:** gcc -pthread -o sleeping_ta sleeping_ta.c \
**Run:** ./sleeping_ta

## Configuration
You can toggle whether students leave after one help session or keep re-queuing by modifying the ONE_AND_DONE macro:\
#define ONE_AND_DONE 1 // 1 = leave after help, 0 = re-queue

## Sample Output
Please enter the number of students being helped.\
\
5\
     Student 1 is on the way to TA office\
     Student 1 is waiting seated at hallway\
     TA is helping Student 1\
     Session finished with Student 1.\
...\
Help Summary:\
Student 1: Helped\
Student 2: Helped\
...\
Simulation ended.\

## Cleanup
All semaphores, mutexes, and dynamically allocated memory are properly destroyed or freed at the end of the simulation

## File Structure
**README.md** - Project Documentation\
**SLEEPING TA 3.ppt** - Powerpoint Presentation explaining the Sleeping TA problem and solution\
**sleeping-ta.c** - Main simulation source code in C language

## Concepts Demonstrated
Producer-consumer synchronization\
Circular queue management\
Thread-safe shared state\
Use of semaphores and mutexes\
Atomic operations for thread coordination\

## Authors
This project was created by Donald, Juliana, and Justin for our CSC385 Operating System Architecture class.
