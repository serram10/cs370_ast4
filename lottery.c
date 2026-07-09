#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>

/* ------------------------ Constants ------------------------ */

const int NUM_LOTTERIES = 5; //Number of lotteries to hold
const int PARTICIPANTS = 3; //Number of participants per lottery

/* ------------------------ Argument and Container Structs ------------------------ */
//Struct representing the container that holds tickets.
struct container{
    int nextTicket;
    int drawnTicket;
    int* entrantIDsArr; //Array of the entrants' ids
};

//Arguments to the participant function
struct patripantArgs{
    int id;
    struct container* container;
};

//Arguments to the judge function
struct judgeArgs
{
    struct container* container;
};

/* ------------------------ Printing Functions ------------------------ */

void enterLottery(int participant_id, int ticket_num)
{
    printf("Participant [%d] has entered the lottery with ticket [%d]\n", participant_id, ticket_num);
}

void startLottery()
{
    printf("Enough participants have entered, starting the lottery...\n");
}

void drawLottery(int ticket_num, int participant_id)
{
    printf("Ticket [%d] was drawn, owned by participant [%d]\n", ticket_num, participant_id);
}

void expressJoy(int participant_id)
{
    printf("[%d] is feeling good!\n", participant_id);
}

void expressSad(int participant_id)
{
    printf("[%d] :(\n", participant_id);
}

void thankParticipants()
{
    printf("Thanks for playing everyone! Until next time...\n");
}

/* ------------------------ Thread Functions ------------------------ */

//TODO: (optionally) Add any locks, semaphores, or barriers here.
//TODO: Add any shared variables for the scenario at global scope
//TODO: Write the thread functions for the judge and participant

pthread_mutex_t lock;       // protect data

sem_t canEnter;             // entering
sem_t judgeStart;           // judge is entering
sem_t winnerChosen;         // winner is chosen
sem_t everyoneDone;         

int entered = 0, reacted = 0; // initializing



void* participant(void* args)
{
    struct patripantArgs* pArgs = (struct patripantArgs*)args;
    int myTicket;

    sem_wait(&canEnter);        // Wait til to enter

    pthread_mutex_lock(&lock);

    // Get ticket and save id
    myTicket = pArgs->container->nextTicket;
    pArgs->container->entrantIDsArr[myTicket] = pArgs->id;
    pArgs->container->nextTicket++;

    enterLottery(pArgs->id, myTicket);

    entered++;

    if(entered == PARTICIPANTS)  // last participants, judge starts
        sem_post(&judgeStart);

    pthread_mutex_unlock(&lock);

    sem_wait(&winnerChosen);     // here we pick a winner

    pthread_mutex_lock(&lock);

    // React to the result
    if(pArgs->container->drawnTicket == myTicket)
        expressJoy(pArgs->id);
    else
        expressSad(pArgs->id);

    reacted++;

    if(reacted == PARTICIPANTS) // everyone reacted then the judge will know
        sem_post(&everyoneDone);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void* judge(void* args)
{
    struct judgeArgs* jArgs = (struct judgeArgs*)args;
    int lotteriesDone = 0;

    while(true)
    {
        sem_wait(&judgeStart);  // waiting for part to enter

        startLottery();

        jArgs->container->drawnTicket = rand() % PARTICIPANTS; // pick rando winner

        drawLottery(   // ticket winner print 
            jArgs->container->drawnTicket,
            jArgs->container->entrantIDsArr[jArgs->container->drawnTicket]
        );

        for(int i = 0; i < PARTICIPANTS; i++)  // part react
            sem_post(&winnerChosen);

        sem_wait(&everyoneDone);  // done reacting

        thankParticipants(); // print

        pthread_mutex_lock(&lock);   // reset for next round 
        entered = 0;
        reacted = 0;
        jArgs->container->nextTicket = 0;
        pthread_mutex_unlock(&lock);

        lotteriesDone++;


        if(lotteriesDone == NUM_LOTTERIES)
            break;


        for(int i = 0; i < PARTICIPANTS; i++)
            sem_post(&canEnter);
    }

    return NULL;
}

/* -------------------------------------- Main --------------------------------------- */

int main(void){
    struct container container; //The single container shared by all threads
    container.nextTicket = 0; //Initial ticket is 0
    container.entrantIDsArr = (int*)malloc(PARTICIPANTS*sizeof(int));

    pthread_t judgeThread;
    struct judgeArgs jArgs;
    jArgs.container = &container;

    // Adding my own code here 

    pthread_mutex_init(&lock, NULL);            // mutex 

    // all of these have to be inialize
    sem_init(&canEnter, 0, PARTICIPANTS);       // 1st group 
    sem_init(&judgeStart, 0, 0);                
    sem_init(&winnerChosen, 0, 0);
    sem_init(&everyoneDone, 0, 0);



    pthread_create(&judgeThread, NULL, judge, &jArgs);

    //Create all of the participant threads and sleep at random intervals
    const int TOTAL_PARTICIPANTS = NUM_LOTTERIES * PARTICIPANTS;
    int numBeforeSleep = rand() % TOTAL_PARTICIPANTS;
    pthread_t participantThreads[TOTAL_PARTICIPANTS];
    struct patripantArgs argsArr[TOTAL_PARTICIPANTS];


    for(int i = 0; i < TOTAL_PARTICIPANTS; ++i)
    {
        argsArr[i].id = i; //Set participant id
        argsArr[i].container = &container;
        pthread_create(&participantThreads[i], NULL, participant, &argsArr[i]); //Launch thread

        //Sleep at random intervals
        if(numBeforeSleep == i)
        {
            sleep(1);
            numBeforeSleep = i + (rand() % (TOTAL_PARTICIPANTS - i)) + 1;
        }
    }


    for(int i = 0; i < TOTAL_PARTICIPANTS; ++i)   // waiting for all part to finish
        pthread_join(participantThreads[i], 0);

    pthread_join(judgeThread, 0);

    free(container.entrantIDsArr); //Deallocate array
}