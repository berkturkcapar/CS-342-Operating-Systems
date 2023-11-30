#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>  
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <unistd.h> //usleep(micrseconds) so 1,000,000 is equal to 1 second
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/signal.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>


// GLOBAL DEFINITIONS
#define MAX_ALLP 2000
#define READY 0
#define RUNNING 1

// STRUCTS
struct pcb {
	int pid;
	int tid;
	int state;	// READY OR RUNNING
    // CV
    int priority;
    int weight;
    double virtualTime;
	int processLength;
	int totalTimeSpentInCPU;
    int nextTimeSlice;
	int arrivalTime;
	int finishTime;
    int contextSwitch;
};

struct node
{
    struct pcb *pcb;
    struct node *next;
};

// GLOBAL CONSTANTS
const int prio_to_weight[40] = {
    88761,  71755,  56483,  46273,  36291,
    29154,  23254,  18705,  14949,  11916,
    9548,   7620,   6100,   4904,   3906,
    3121,   2501,   1991,   1586,   1277,
    1024,   820,    655,    526,    423,
    335,    272,    215,    172,    137,
    110,    87,     70,     56,     45,
    36,     29,     23,     18,     15
};
const int SCHED_LATENCY = 100;
const int GRANULARITY  = 10;
const int WEIGHT_0 = 1024;
const char INPUT_CLI[] = "C";
const char INPUT_FILE[] = "F";
enum inputType{C,F};
const char FIXED[] = "fixed";
const char UNIFORM[] = "uniform";
const char EXPONENTIAL[] = "exponential";
enum distType{fixed, uniform, expo};

// GLOBAL VARIABLES
long sysStart;
char *infile;
FILE *fptr;
enum inputType inputType;
int minPrio;
int maxPrio;
enum distType plType;
int minPL;
int maxPL;
int avgPL;
enum distType iatType;
int minIAT;
int maxIAT;
int avgIAT;
int rqLen;
int allp;
int outmode;
bool outfileSpecified;
char *outfile;
int current, generatorFinished;  
struct node *head, *head2; // head2 will hold the list of terminated PCBs
int executedThreads;
pthread_t generator, scheduler;	
pthread_t threads[MAX_ALLP];	
pthread_mutex_t cpu , lock_queue, lock_current; 
pthread_cond_t cvScheduler;
pthread_cond_t cvThreadProcesses[MAX_ALLP];

// QUEUE METHODS
double printTable(struct node *head2){
    struct node *ptr = head2;
    if(ptr != NULL) {
        if (ptr ->next != NULL) 
		{
            ptr = ptr ->next; // skip the first dead node
        }

    } 
	else {
        printf("Empty list from print list\n");
        return;
    }

    int sum = 0;
    int count = 0;
    if(outfileSpecified) 
        fprintf(fptr, "\npid\t\tarv\t\tdept\t\tprio\t\tcpu\t\twaitr\t\tturna\t\tcs\n");
    else
        printf( "\npid\tarv\tdept\tprio\tcpu\twaitr\tturna\tcs\n");
    
    // start from the beginning
    while (ptr != NULL)
    {
        int turnaround =  ptr->pcb->finishTime - ptr->pcb->arrivalTime;
        int myWait = turnaround - ptr->pcb->totalTimeSpentInCPU;

        if(outfileSpecified) 
            fprintf(fptr, "%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
            ptr->pcb->pid +1 ,
            ptr->pcb->arrivalTime,
            ptr->pcb->finishTime,
            ptr->pcb->priority,
            ptr->pcb->totalTimeSpentInCPU,
            myWait,
            turnaround,
            ptr->pcb->contextSwitch
            );
        else
            printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
            ptr->pcb->pid + 1,
            ptr->pcb->arrivalTime,
            ptr->pcb->finishTime,
            ptr->pcb->priority,
            ptr->pcb->totalTimeSpentInCPU,
            myWait,
            turnaround,
            ptr->pcb->contextSwitch
            );
        
        sum +=  myWait;
        count++;
        
        ptr = ptr->next;
    }
    return (double) sum / (double) count;
   
}

int getNextPID(struct node *head){
    struct node *ptr = head;
    if(ptr != NULL) {
        if (ptr ->next != NULL) 
            ptr = ptr ->next; // skip the first dead node
    } 
	else 
        return -1;

    return ptr->pcb->pid;
}

int calculateNextTimeSlice(struct node *head){
    struct node *ptr = head;
    if(ptr != NULL) {
        if (ptr ->next != NULL) 
            ptr = ptr ->next; // skip the first dead node
    } 
	else 
        return -1;

    int weightK = ptr->pcb->weight;
    int sumWeight = 0;
    // start from the beginning
    while (ptr != NULL){
        sumWeight += ptr->pcb->weight;
        ptr = ptr->next;
    }

    double ratio = (double) weightK /  (double) sumWeight;
    int newTimeSlice = ratio * SCHED_LATENCY;
    int res = newTimeSlice;

    if( newTimeSlice <= GRANULARITY)
        res = GRANULARITY;

    return res;
}

struct pcb* createPcb(int pid, int tid, int prio,  int processLength, double vrTime,
    int totalTimeSpentInCPU,  int nextTimeSlice, int arrivalTime,
     int finishTime , int contextSwitch ) {
	struct pcb *pcb = (struct pcb*)malloc(sizeof(struct pcb));
	pcb->pid = pid;
	pcb->tid = tid;
    pcb->state = READY;
    pcb->priority = prio;
    pcb->weight = prio_to_weight[ prio + 20];
    pcb->virtualTime = vrTime;	
    pcb->processLength = processLength;
    pcb->totalTimeSpentInCPU = totalTimeSpentInCPU;
	pcb-> nextTimeSlice= nextTimeSlice  ;
	pcb->arrivalTime = arrivalTime;
	pcb->finishTime = finishTime;
    pcb->contextSwitch = contextSwitch;

	return pcb;
}

struct pcb* popFirst(struct node **head_ref, struct node *head){
    struct node *ptr = head;
    if(ptr == NULL) 
        return NULL;
    else {
        if (ptr ->next != NULL) ptr = ptr ->next; // skip the first dead node
        struct pcb* res = ptr->pcb;
        // if this was the last element, then set the head as null
        if(ptr->next == NULL) (*head_ref) = NULL;
        else head->next = ptr->next;
        return ptr->pcb;
    } 
}

void insertNode(struct node **head_ref, struct node *head,
    int pid, int tid, int prio,  int processLength, double vrTime,
    int totalTimeSpentInCPU,  int nextTimeSlice, int arrivalTime,
    int finishTime , int contextSwitch){
    // insert the first node
    if ((*head_ref) == NULL){
        struct node *deadNode = (struct node *)malloc(sizeof(struct node));
        deadNode->pcb =  createPcb(-1,-1,0,-1,-1.0,-1,-1,-1,-1, -1);

        struct node *firstNode = (struct node *)malloc(sizeof(struct node));
        firstNode->pcb =  createPcb(  
            pid,  
            tid,  
            prio,   
            processLength,  
            vrTime,
            totalTimeSpentInCPU,  
            nextTimeSlice,  
            arrivalTime,
            finishTime,
            contextSwitch);

        deadNode->next = firstNode;
        firstNode->next = NULL;

        (*head_ref) = deadNode;
        return;
    } else {
        struct node *nextNode = head->next;
        while (nextNode != NULL) {
            if (vrTime < nextNode->pcb->virtualTime)
            { // we need to insert just before the nextNode

                struct node *newNode = (struct node *)malloc(sizeof(struct node));
                newNode->pcb =  createPcb(  
                    pid,  
                    tid,  
                    prio,   
                    processLength,  
                    vrTime,
                    totalTimeSpentInCPU,  
                    nextTimeSlice,  
                    arrivalTime,
                    finishTime,
                    contextSwitch );
                head->next = newNode;
                newNode->next = nextNode;
                int time = getCurrSysTime();
                return;
            }

            // increment the ptrs
            head = nextNode;
            nextNode = nextNode->next;
        }

        // if so far the function did not return, the new word goes to the end
        struct node *newNode = (struct node *)malloc(sizeof(struct node));
        newNode->pcb = createPcb(  
                    pid,  
                    tid,  
                    prio,   
                    processLength,  
                    vrTime,
                    totalTimeSpentInCPU,     
                    nextTimeSlice,  
                    arrivalTime,
                    finishTime,
                    contextSwitch);
        newNode->next = NULL;
        head->next = newNode;
        int time = getCurrSysTime();
        return;
    }
}

void deleteList(struct node* head) {
    struct node* temp;
    while(head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

// HELPER METHODS

double randfrom(double min, double max) {
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

int generateExponential( int min, int max, int avg) {
    double mu = (double)(1.0 / avg);
    double u = (double) randfrom(0.0, 1.0);
    double resLog = log((1.0 - u));
    double x = (-1.0 *resLog)/mu;

    while( x >= max || x <= min) {
        u = (double) randfrom(0.0, 1.0);
        resLog = log((1.0 - u));
        x = (-1.0 *resLog)/mu;
    }
    return (int)(x+0.5);     
}

int getCurrSysTime () {
    struct timeval nowTV;
    struct timezone nowTZ;
    gettimeofday(&nowTV, &nowTZ);
    long now  = nowTV.tv_sec * 1000 + nowTV.tv_usec / 1000; 
    return (now - sysStart);
}

int getNextIAT() {
    int nextIAT;
    if( iatType == fixed) { nextIAT = avgIAT;}
    else if( iatType == uniform) {
        int r = rand() % (maxIAT - minIAT + 1 ) ;
        nextIAT = minIAT + r;
    } else { // iatType == expo
        nextIAT = generateExponential( minIAT, maxIAT, avgIAT);
    }
    return nextIAT;
}

int getNextPL() {
    // determine the process length (CPU burst)
    int nextPL;
    if( plType == fixed) { nextPL = avgPL;}
    else if( plType == uniform) {
        int r = rand() % (maxPL- minPL + 1);
        nextPL = minPL + r;
    } else { // plType == expo
        nextPL = generateExponential( minPL, maxPL, avgPL);
    }

    return nextPL;
}


// THREAD METHODS
static void *do_scheduler(void *arg_ptr) {
    int time;
    int i = 0;
    
    // we need cpu to be available first
    pthread_mutex_lock(&cpu);

    while (executedThreads != allp || generatorFinished != 1 ){
        while(current != -1) 
            pthread_cond_wait(&cvScheduler, &cpu);
        
        pthread_mutex_lock(&lock_queue);
        int nextToBeExec = getNextPID(head);
        pthread_mutex_unlock(&lock_queue);
        
        if(outmode==3) {
            time = getCurrSysTime();
            if(nextToBeExec != -1) {
                if(outfileSpecified) 
                    fprintf(fptr, "%d ms/ SCHEDULER : The next thread to be executed is %d!\n", time, nextToBeExec +1);
                else
                    printf( "%d ms/ SCHEDULER : The next thread to be executed is %d!\n", time, nextToBeExec +1);
            }
            
        }

        pthread_mutex_lock(&lock_current);
        current = nextToBeExec;
        pthread_mutex_unlock(&lock_current);

        pthread_cond_signal(&(cvThreadProcesses[nextToBeExec]));
        
        time = getCurrSysTime();
    }

    pthread_exit(0);

}

static void *do_process_thread(void *param) {
    int time;
    int pid = *((int *)param);

    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms / THREAD %d: I am created\n", time, pid +1 );
        else
            printf("%d ms / THREAD %d: I am created\n", time, pid + 1 );
    }

    bool threadIsDone = false;
     
    int determined_priority = rand() % (maxPrio + 1 - minPrio);
    determined_priority += minPrio;  //  This way, priority is unifromly distributed in the interval [-20, 19]
    int arrival = getCurrSysTime();
    int nextPL = getNextPL();
    insertNode(&head, head, pid, threads[pid], determined_priority, nextPL, 0.0, 0, 0, arrival,-1, 0 );
    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms / THREAD %d: I am added to queue\n", time, pid +1 );
        else
            printf("%d ms / THREAD %d: I am added to queue\n", time, pid  + 1);
    }

    // we need cpu to be available first
    pthread_mutex_lock(&cpu);
    while(threadIsDone == false) {
        time = getCurrSysTime();
        while(current != pid) 
            pthread_cond_wait(&(cvThreadProcesses[pid]), &cpu);

        // determine next timeslice
        pthread_mutex_lock(&lock_queue);
        int nextTS = calculateNextTimeSlice(head);
   
        // Pop pcb
        struct pcb *myPCB = popFirst(&head, head);
        pthread_mutex_unlock(&lock_queue);

        if((myPCB->processLength - myPCB->totalTimeSpentInCPU ) < nextTS)
            nextTS = (myPCB->processLength - myPCB->totalTimeSpentInCPU );    
   
        if(outmode == 3) {
            time = getCurrSysTime();
            if(outfileSpecified) 
                fprintf(fptr, "%d ms/ THREAD %d: I am starting to be executed on CPU for %d milliseconds!\n", time, pid +1, nextTS);
            else
                printf( "%d ms/ THREAD %d: I am starting to be executed on CPU for %d milliseconds!\n", time, pid+1, nextTS);
        } 
        if( outmode == 2) {
            if(outfileSpecified) 
                fprintf(fptr, "%d\t%d\tRUNNING\t\t%d\n", time, pid +1, nextTS);
            else
                printf("%d\t%d\tRUNNING\t\t%d\n", time, pid +1, nextTS);
        } 
        usleep(nextTS*1000);
 
        if(outmode == 3) {
            time = getCurrSysTime();
            if(outfileSpecified) 
                fprintf(fptr, "%d ms/ THREAD %d: I am done with this timeslice!\n", time, pid +1);
            else
                printf( "%d ms/ THREAD %d: I am done with this timeslice!\n", time, pid+1);
        } 
      
        // Set current as -1 (so scheduler knows now is its turn) and signal scheduler
        pthread_mutex_lock(&lock_current);
        current = -1;
        pthread_mutex_unlock(&lock_current);

        //insertNode(&head, head, pid)
        double newVR = myPCB->virtualTime +  ((double) WEIGHT_0 / myPCB->weight)*nextTS; 
        int newTotalTimeSpentInCPU = myPCB->totalTimeSpentInCPU + nextTS;

        if( myPCB->processLength <= newTotalTimeSpentInCPU) {
            // The process is finished, it should terminate
            threadIsDone = true;
            int finTime = getCurrSysTime();
            insertNode(&head2, head2, myPCB->pid, myPCB->tid, myPCB->priority, 
            myPCB->processLength, (double)pid, newTotalTimeSpentInCPU, 0, myPCB->arrivalTime, finTime, myPCB->contextSwitch +1);
    
        } else {
            pthread_mutex_lock(&lock_queue);
            insertNode(&head, head, myPCB->pid, myPCB->tid, myPCB->priority, 
            myPCB->processLength, newVR, newTotalTimeSpentInCPU, 0, myPCB->arrivalTime, -1, myPCB->contextSwitch +1);
            pthread_mutex_unlock(&lock_queue);
            if(outmode == 3) {
                time = getCurrSysTime();
                if(outfileSpecified) 
                    fprintf(fptr, "%d ms / THREAD %d: I am added to queue\n", time, pid +1);
                else
                    printf("%d ms / THREAD %d: I am added to queue\n", time, pid +1);
            }
        }

        pthread_cond_signal(&cvScheduler);

    }

    executedThreads++;
    pthread_mutex_unlock(&cpu);

    // Exit
    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms/ THREAD %d: I terminate! This means I don't have any more timeslices\n", time, pid+1);
        else
            printf( "%d ms/ THREAD %d: I terminate! This means I don't have any more timeslices\n", time, pid+1);
    } 

    pthread_exit(0);
    
}

static void *generatorCLI(void *arg_ptr) {

    executedThreads = 0;
    int time;
    int createdProcessNo = 0;

    while( createdProcessNo < allp) {
        // determine the next interval arrival time       
        int nextIAT = getNextIAT();

        int *pid = (int*)malloc(sizeof(int));
		*pid = (createdProcessNo);
    
        pthread_create(&threads[createdProcessNo % 3], NULL, do_process_thread, pid);
    
        time = getCurrSysTime();
        usleep(nextIAT*1000);
        
        createdProcessNo++;

    }

    time = getCurrSysTime();
      generatorFinished = 1;

    for (int i = 0; i < allp; i++) {
		pthread_join(threads[i], NULL);
	}

    time = getCurrSysTime();
    pthread_exit(0);

}

static void *do_process_thread_file(void *param) {
    int time;
    int *info = (int *) param;
    int pid = info[0];
    int nextPL = info[1];
    int determined_priority = info[2];

 
    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms / THREAD %d: I am created\n", time, pid +1 );
        else
            printf("%d ms / THREAD %d: I am created\n", time, pid+1 );
    }

    bool threadIsDone = false;
    int arrival = getCurrSysTime();
    insertNode(&head, head, pid, threads[pid], determined_priority, nextPL, 0.0, 0, 0, arrival,-1 , 0);
    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms / THREAD %d: I am added to queue\n", time, pid+1 );
        else
            printf("%d ms / THREAD %d: I am added to queue\n", time, pid +1);
    }

    // we need cpu to be available first
    pthread_mutex_lock(&cpu);
    while(threadIsDone == false) {
        time = getCurrSysTime();
        while(current != pid) 
            pthread_cond_wait(&(cvThreadProcesses[pid]), &cpu);

        // determine next timeslice
        pthread_mutex_lock(&lock_queue);
        int nextTS = calculateNextTimeSlice(head);
   
        // Pop pcb
        struct pcb *myPCB = popFirst(&head, head);
        pthread_mutex_unlock(&lock_queue);

        if((myPCB->processLength - myPCB->totalTimeSpentInCPU ) < nextTS)
            nextTS = (myPCB->processLength - myPCB->totalTimeSpentInCPU );

        if(outmode == 3) {
            time = getCurrSysTime();
            if(outfileSpecified) 
                fprintf(fptr, "%d ms/ THREAD %d: I am starting to be executed on CPU for %d milliseconds!\n", time, pid+1, nextTS);
            else
                printf( "%d ms/ THREAD %d: I am starting to be executed on CPU for %d milliseconds!\n", time, pid+1, nextTS);
        }      
        
        if( outmode == 2) {
            if(outfileSpecified) 
                fprintf(fptr, "%d\t%d\tRUNNING\t\t%d\n", time, pid+1, nextTS);
            else
                printf("%d\t%d\tRUNNING\t\t%d\n", time, pid+1, nextTS);
            
        } 
        
        usleep(nextTS*1000);

        if(outmode == 3) {
            time = getCurrSysTime();
            if(outfileSpecified) 
                fprintf(fptr, "%d ms/ THREAD %d: I am done with this timeslice!\n", time, pid+1);
            else
                printf( "%d ms/ THREAD %d: I am done with this timeslice!\n", time, pid+1);
        } 
      

        // Set current as -1 (so scheduler knows now is its turn) and signal scheduler
        pthread_mutex_lock(&lock_current);
        current = -1;
        pthread_mutex_unlock(&lock_current);

        //insertNode(&head, head, pid)
        double newVR = myPCB->virtualTime +  ((double) WEIGHT_0 / myPCB->weight)*nextTS; 
        int newTotalTimeSpentInCPU = myPCB->totalTimeSpentInCPU + nextTS;

        if( myPCB->processLength <= newTotalTimeSpentInCPU) {
            // The process is finished, it should terminate
            threadIsDone = true;
            int finTime = getCurrSysTime();
            insertNode(&head2, head2, myPCB->pid, myPCB->tid, myPCB->priority, 
            myPCB->processLength, (double)pid, newTotalTimeSpentInCPU, 0, myPCB->arrivalTime, finTime, myPCB->contextSwitch +1);
    
        } else {
            pthread_mutex_lock(&lock_queue);
            insertNode(&head, head, myPCB->pid, myPCB->tid, myPCB->priority, 
            myPCB->processLength, newVR, newTotalTimeSpentInCPU, 0, myPCB->arrivalTime, -1, myPCB->contextSwitch +1);
            if(outmode == 3) {
                time = getCurrSysTime();
                if(outfileSpecified) 
                    fprintf(fptr, "%d ms / THREAD %d: I am added to queue\n", time, pid+1 );
                else
                    printf("%d ms / THREAD %d: I am added to queue\n", time, pid +1);
            }
            
            pthread_mutex_unlock(&lock_queue);
        }
        

        pthread_cond_signal(&cvScheduler);

    }

    executedThreads++;

    pthread_mutex_unlock(&cpu);

    

    // Exit
    if(outmode == 3) {
        time = getCurrSysTime();
        if(outfileSpecified) 
            fprintf(fptr, "%d ms/ THREAD %d: I terminate! This means I don't have any more timeslices\n", time, pid+1);
        else
            printf( "%d ms/ THREAD %d: I terminate! This means I don't have any more timeslices\n", time, pid+1);
    } 
  
    pthread_exit(0);
    
}

static void *generatorFile(void *arg_ptr) {

    executedThreads = 0;

    int time;
    int createdProcessNo = 0;

    //printf("rqLen: %d allp: %d outmode: %d\n", rqLen, allp, outmode);
    //printf("infile: %s\n", infile);
    FILE *fp;
	fp = fopen(infile, "r");
	if (fp == NULL)
	{
		perror("File could not be opened\n");
		exit(1);
	}
    else {

        char word[64];
        int nextIAT = 0;
        int infoForThread[3];
       
        while (createdProcessNo < allp) 
        {
            if (fscanf(fp, "%63s", word) == 1)
            {
                infoForThread[0] = createdProcessNo;
                fscanf(fp, "%63s", word);
                infoForThread[1] = atoi(word);
                fscanf(fp, "%63s", word);
                infoForThread[2] = atoi(word);
            
                pthread_create(&threads[createdProcessNo ], NULL, do_process_thread_file, (void*)infoForThread);
            }
            if (fscanf(fp, "%63s", word) == 1)
            {
                fscanf(fp, "%63s", word);
                nextIAT = atoi(word);
                
            }
            time = getCurrSysTime();
            usleep(nextIAT*1000);
            createdProcessNo = createdProcessNo + 1;
        }
       
        fclose(fp);
    }

    time = getCurrSysTime();
    generatorFinished = 1;

    for (int i = 0; i < allp; i++) {
		pthread_join(threads[i], NULL);
	}

    time = getCurrSysTime();
    pthread_exit(0);

}

// MAIN
int main(int argc, char *argv[]) {

    if( strcmp(INPUT_CLI, argv[1] ) == 0) {
        // cfs C minPrio maxPrio distPL avgPL minPL maxPL distIAT avgIAT minIAT maxIAT 
        //rqLen ALLP OUTMODE [OUTFILE]
        minPrio = atoi(argv[2]);
        maxPrio = atoi(argv[3]);

        if(strcmp(FIXED, argv[4]) == 0) { plType = fixed;}
        else if(strcmp(UNIFORM, argv[4]) == 0) { plType = uniform; } 
        else if( strcmp(EXPONENTIAL, argv[4]) == 0) { plType = expo;} 
        else { printf("Wrong input format!\n"); return -1; }
        avgPL = atoi(argv[5]);
        minPL = atoi(argv[6]);
        maxPL = atoi(argv[7]);

        if(strcmp(FIXED, argv[8]) == 0) { iatType = fixed;}
        else if(strcmp(UNIFORM, argv[8]) == 0) { iatType = uniform; } 
        else if( strcmp(EXPONENTIAL, argv[8]) == 0) { iatType = expo;} 
        else { printf("Wrong input format!\n"); return -1; }
        avgIAT = atoi(argv[9]);
        minIAT = atoi(argv[10]);
        maxIAT = atoi(argv[11]);

        rqLen = atoi(argv[12]);
        allp = atoi(argv[13]);
        outmode = atoi(argv[14]);

        if(argc == 16) { outfileSpecified = true; outfile = argv[15];}
        else { outfileSpecified = false;}

        // Some edge cases where input format is wrong
        if(minPrio < -20 || maxPrio > 19 || minPL > maxPL || minIAT > maxIAT) {printf("Wrong input format!\n"); return -1;}

    } else if(strcmp(INPUT_FILE, argv[1] ) == 0) {
        //cfs F rqLen ALLP OUTMODE INFILE [OUTFILE]
        rqLen = atoi(argv[2]);
        allp = atoi(argv[3]);
        outmode = atoi(argv[4]);
        infile = argv[5];
        if(argc == 7) { outfileSpecified = true; outfile = argv[6];}
        else { outfileSpecified = false;}

    } else { printf("Wrong input format!\n"); return -1;}

    struct timeval systemStartTV;
    struct timezone systemStartTZ;
    gettimeofday(&systemStartTV, &systemStartTZ);
    sysStart = systemStartTV.tv_sec * 1000 + systemStartTV.tv_usec / 1000 ; 

    head = NULL;
    head2 = NULL;
    int r;

    if(outfileSpecified) {
        fptr = fopen(outfile, "w");
        if (fptr == NULL) {
            printf("Error opening the file %s*n", outfile);
            return -1;
        }
    }

    pthread_mutex_init(&cpu, NULL);
    pthread_mutex_init(&lock_current, NULL);
    pthread_mutex_init(&lock_queue, NULL);
    pthread_cond_init(&cvScheduler, NULL);

    r = pthread_create(&scheduler, NULL, do_scheduler, NULL);
	if (r != 0) { printf("thread create failed \n"); exit(1);}

    if (strcmp(INPUT_CLI, argv[1] ) == 0) {
        r = pthread_create(&generator, NULL, generatorCLI, NULL);
	    if (r != 0) { printf("thread create failed \n"); exit(1);}
    }

    if (strcmp(INPUT_FILE, argv[1] ) == 0){
        r = pthread_create(&generator, NULL, generatorFile, NULL);
	    if (r != 0) { printf("thread create failed \n"); exit(1);}
    }
    // --------

    r = pthread_join(generator, NULL);
    if (r != 0) { printf("thread join failed \n"); exit(1);}

    r = pthread_join(scheduler, NULL);
    if (r != 0) { printf("thread join failed \n"); exit(1);}

    pthread_cond_destroy(&cvScheduler);
    pthread_mutex_destroy(&lock_queue);
    pthread_mutex_destroy(&cpu);
    pthread_mutex_destroy(&lock_current);
   
    double avgWait = printTable (head2);
    if(outfileSpecified) 
        fprintf(fptr, "avg waiting time: %f\n" , avgWait);
    else 
        printf("avg waiting time: %f\n" , avgWait);

    deleteList(head);
    deleteList(head2);
    if (outfileSpecified)
        fclose(fptr);

    return 1;

}
