
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#define MAXTHREADS  8		// max number of threads
#define MAXFILENAME 50		// max length of a filename

// thread function will take a pointer to this structure

struct node {
    char *word;
    int freq;
    struct node *next;
};

struct node* head = NULL;

void printList(struct node *head)
{
    struct node *ptr = head;
    if(ptr != NULL) 
	{
        if (ptr ->next != NULL) 
		{
            ptr = ptr ->next; // skip the first dead node
        }

    } 
	else 
	{
        printf("Empty List\n");
        return;
    }

    // start from the beginning
    while (ptr != NULL)
    {
        printf("%s %d\n", ptr->word, ptr->freq);
        ptr = ptr->next;
    }
}

void insertNode(struct node **head_ref, struct node *head, char *word)
{

    // insert the first node
    if ((*head_ref) == NULL)
    {
        struct node *deadNode = (struct node *)malloc(sizeof(struct node));
        deadNode->word = strdup(" ");
        deadNode->freq = 0;

        struct node *firstNode = (struct node *)malloc(sizeof(struct node));
        firstNode->word = strdup(word);
        firstNode->freq = 1;

        deadNode->next = firstNode;
        firstNode->next = NULL;

        (*head_ref) = deadNode;
        return;
    }
    else
    {

        struct node *nextNode = head->next;

        while(nextNode != NULL) 
		{
            
            if(strcmp( word, nextNode->word) == 0) { // we found the same word
                nextNode->freq = nextNode->freq + 1;
                return;
            }
            
            if( strcmp( word, nextNode->word) < 0) { // we need to insert just before the nextNode
                
                struct node *newNode = (struct node *)malloc(sizeof(struct node));
                newNode->word = strdup(word);
                newNode->freq = 1;
                head->next = newNode;
                newNode->next = nextNode;
                return;
            }  
            
            // increment the ptrs
            head = nextNode;
            nextNode = nextNode->next;
            
        }

        // if so far the function did not return, the new word goes to the end
        struct node *newNode = (struct node *)malloc(sizeof(struct node));
        newNode->word = strdup(word);
        newNode->freq = 1;
        newNode->next = NULL;
        head->next = newNode;
        return;

    }
}

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr)
{
	char *retreason; 

	FILE *fp;
	fp = fopen((char*)arg_ptr, "r");
	if (fp == NULL)
	{
		perror("File could not be opened\n");
		exit(1);
	}

	char word[64];
	
	while (fscanf(fp, "%63s", word) == 1) 
	{
		for (int i = 0; i < strlen(word); i++) 
		{
        	word[i] = toupper(word[i]);
		}
		insertNode(&head, head, word);
    }

	fclose(fp);
	
	retreason = (char *) malloc (200);
	strcpy (retreason, "normal termination of thread"); 
	pthread_exit(retreason); //  tell a reason to thread waiting in join
    // we could simple exit as below, if we don't want to pass a reason
    // pthread_exit(NULL);
    // then we would also modify pthread_join call.
}

void createOutputFile(char* fileName)
{
	FILE *fp;
	fp = fopen(fileName, "w");
	struct node *ptr = head;
	if(ptr != NULL) 
	{
        if (ptr ->next != NULL) 
		{
            ptr = ptr ->next; // skip the first dead node
        }
    }
	if (fp == NULL) 
	{
		perror("do_task:");
		exit(1);
	}
	else 
	{
		// start from the beginning
		while (ptr != NULL)
		{
			fprintf(fp,"%s %d", ptr->word, ptr->freq);
			fprintf(fp,"\n");
			ptr = ptr->next;
		}
	}
	fclose(fp);
}

int main(int argc, char *argv[])
{
	// struct timeval beforeTime;
   	// struct timeval afterTime;

	// gettimeofday(&beforeTime, NULL);
	// printf("\n*********Start of tword**********\n");
	// printf("gettimeofday in ms: %ld\n", beforeTime.tv_usec);
	// printf("***********************************\n");
	int count;		        // number of threads
	int i;
	int ret;
	char *retmsg; 

	if (argc < 4) {
		printf ("usage: tword <outfile> <N> <infile1> <infile2> ... <infileN>\n");
		exit(1);
	}

	count = atoi(argv[2]);	// number of threads to create
	pthread_t tids[count];	// thread ids

	for (i = 0; i < count; ++i) {
		ret = pthread_create(&(tids[i]),
				     NULL, do_task, argv[i + 3]);

		if (ret != 0) {
			exit(1);
		}
	}

	for (i = 0; i < count; ++i) {
	    ret = pthread_join(tids[i], (void **)&retmsg);
		if (ret != 0) {
			exit(1);
		}
		// we got the reason as the string pointed by retmsg.
		// space for that was allocated in thread function.
        // now we are freeing the allocated space.
		free (retmsg);
	}
	// printList(head);
	createOutputFile(argv[1]);
	// gettimeofday(&afterTime, NULL);
	// printf("***********End of tword************\n");
	// printf("gettimeofday in ms: %ld\n", afterTime.tv_usec);
	// printf("***********************************\n");
	// printf("elapsed time in ms: %ld\n", (afterTime.tv_usec - beforeTime.tv_usec));
	// printf("***********************************\n");
	return 0;
}
