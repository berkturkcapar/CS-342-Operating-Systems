#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#define MQNAME "/proje"

// Internal data structure to keep the words and their frequencies
struct node
{
    char *word;
    int freq;
    int word_len;
    struct node *next;
};

// This will be a linked-list / priority queueish structure. As you will add
// new node in it, the node will be placed to the correct place so that it won't
// be needing additional sorting. The frequency will be incremented as new word is added

void insertNode(struct node **head_ref, struct node *head, char *word)
{

    // insert the first node
    if ((*head_ref) == NULL)
    {
        struct node *deadNode = (struct node *)malloc(sizeof(struct node));
        deadNode->word = strdup(" ");
        deadNode->word_len = 1;
        deadNode->freq = 0;

        struct node *firstNode = (struct node *)malloc(sizeof(struct node));
        firstNode->word = strdup(word);
        firstNode->word_len = strlen(word) + 1;
        firstNode->freq = 1;

        deadNode->next = firstNode;
        firstNode->next = NULL;

        (*head_ref) = deadNode;
        return;
    }
    else
    {

        struct node *nextNode = head->next;

        while (nextNode != NULL)
        {

            if (strcmp(word, nextNode->word) == 0)
            { // we found the same word
                nextNode->freq = nextNode->freq + 1;
                return;
            }

            if (strcmp(word, nextNode->word) < 0)
            { // we need to insert just before the nextNode

                struct node *newNode = (struct node *)malloc(sizeof(struct node));
                newNode->word = strdup(word);
                newNode->word_len = strlen(newNode->word) + 1;
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
        newNode->word_len = strlen(newNode->word) + 1;
        newNode->next = NULL;
        head->next = newNode;
        return;
    }
}

void insertNodeWithFreq(struct node **head_ref, struct node *head, char *word, int givenFreq)
{

    // insert the first node
    if ((*head_ref) == NULL)
    {
        struct node *deadNode = (struct node *)malloc(sizeof(struct node));
        deadNode->word = strdup(" ");
        deadNode->word_len = 1;
        deadNode->freq = 0;

        struct node *firstNode = (struct node *)malloc(sizeof(struct node));
        firstNode->word = strdup(word);
        firstNode->word_len = strlen(word) + 1;
        firstNode->freq = givenFreq;

        deadNode->next = firstNode;
        firstNode->next = NULL;

        (*head_ref) = deadNode;
        return;
    }
    else
    {

        struct node *nextNode = head->next;

        while (nextNode != NULL)
        {

            if (strcmp(word, nextNode->word) == 0)
            { // we found the same word
                nextNode->freq = nextNode->freq + givenFreq;
                return;
            }

            if (strcmp(word, nextNode->word) < 0)
            { // we need to insert just before the nextNode

                struct node *newNode = (struct node *)malloc(sizeof(struct node));
                newNode->word = strdup(word);
                newNode->word_len = strlen(newNode->word) + 1;
                newNode->freq = givenFreq;
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
        newNode->freq = givenFreq;
        newNode->word_len = strlen(newNode->word) + 1;
        newNode->next = NULL;
        head->next = newNode;
        return;
    }
}

char *bufferp; // pointer to receive buffer - allocated with malloc()
int bufferlen; // length of the receive buffer
struct item *itemp;

void deleteList(struct node* head) {
    struct node* temp;
    while(head != NULL) {
        temp = head;
        head = head->next;

        free(temp->word);
        free(temp);
    }
}

void producer(int childNo, char *fileName, int mgsize)
{
    // Read file
    FILE *file;
    file = fopen(fileName, "r");

    char word[64];
    struct node *head = NULL;

    while (fscanf(file, " %63s", word) == 1)
    {
        for (int i = 0; i < strlen(word); i++)
        {
            word[i] = toupper(word[i]);
        }
        insertNode(&head, head, word);
    }

    // close file after it has been read
    fclose(file);
   
    // Open mq
    mqd_t mq;
    struct mq_attr mq_attr;

    mq = mq_open(MQNAME, O_RDWR);
    if (mq == -1)
    {
        perror("can not open msg queue\n");
        exit(1);
    }
   

    char buf[10000];
    int buf_size = 4;
    char *buf_ptr = buf + 4; // the different word count will be added later
    int dw_count = 0;        // the number of different words
    for (struct node *cur = head->next; cur != NULL; cur = cur->next)
    {
        dw_count++;

        size_t cnt = cur->word_len;
        while (cnt % 4 != 0)
        {
            cnt++;
        }

        if (buf_size + 8 + (int)cnt > mgsize)
        {
            dw_count--;

            memcpy(buf, (char *)&(dw_count), 4);
            mq_send(mq, buf, mgsize, 0);
            buf_ptr = buf + 4;
         
            for (int j = 0; j < buf_size; j++)
            {
                buf[j] = 0;
            }
            buf_size = 4;
            dw_count = 0;
        }
        else
        {
            memcpy(buf_ptr, (char *)&(cur->freq), 4);
            buf_ptr = buf_ptr + 4;
            memcpy(buf_ptr, (char *)&cnt, 4);
            buf_ptr = buf_ptr + 4;
            memcpy(buf_ptr, cur->word, cnt);
            buf_ptr = buf_ptr + (int)cnt;
            buf_size = buf_size + 8 + (int)cnt;
        }
    }
    
    // Send the remaining words
    memcpy(buf, (char *)&dw_count, 4);
    int res = mq_send(mq, buf, mgsize, 0);
    if (res == -1)
        perror("Send Error: ");
    
    
    deleteList(head);

    int msgend = -1;
    int res2 = mq_send(mq, (char *)&msgend, mgsize, 0);
    if (res2 == -1)
        perror("end send error");

    mq_close(mq);
    exit(0);
}

void recieve(int total_child_num, int mgsize, char *outfile)
{
    mqd_t mq;
    struct mq_attr mq_attr;

    mq = mq_open(MQNAME, O_RDWR, 0666, NULL);
    if (mq == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
 

    mq_getattr(mq, &mq_attr);

    struct node *head = NULL;

    char buf[10000];
    int no_of_finished_child = 0;
    while (1)
    {
        int er = mq_receive(mq, buf, 10000, 0);
        int isItTheLastMsg = 0;
        memcpy((char *)&isItTheLastMsg, buf, 4);
        if (isItTheLastMsg == -1)
        {
            no_of_finished_child++;
            if (no_of_finished_child == total_child_num)
            {
                break;
            }
        }

        int dw_count = 0;
        memcpy((char *)&dw_count, buf, 4);
        char *buf_ptr = buf + 4;
        for (int k = 0; k < dw_count; k++)
        {
            int count = 0;
            int word_length = 0;
            char word[64];
            memcpy((char *)&count, buf_ptr, 4);
            buf_ptr = buf_ptr + 4;
            memcpy((char *)&word_length, buf_ptr, 4);
            buf_ptr = buf_ptr + 4;
            memcpy(word, buf_ptr, word_length);
            buf_ptr = buf_ptr + word_length;
         
            insertNodeWithFreq(&head, head, word, count);
        }
    }

    // open and write to the output file
    FILE *fp = fopen(outfile, "w+");
    for (struct node *cur = head->next; cur != NULL; cur = cur->next)
    {
        fprintf(fp, "%s %d\n", cur->word, cur->freq);
    }

    deleteList(head);
    fclose(fp);
}


int main(int argc, char *argv[])
{
    // struct timeval beforeTime;
   	// struct timeval afterTime;
    // gettimeofday(&beforeTime, NULL);
	// printf("\n*********Start of pword**********\n");
	// printf("gettimeofday in ms: %ld\n", beforeTime.tv_usec);
	// printf("***********************************\n");
    if (argc < 5)
    {
        perror("Error: Missing input arguments!\n");
        return -1;
    } // according to the definition, there has to be at least 5 inputs

    int mgsize = atoi(argv[1]);
    char *outfile = argv[2];
    int N = atoi(argv[3]);

    mqd_t mq;
    struct mq_attr mq_attr;

    mq = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
    if (mq == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
   
    mq_close(mq);
   
   // the main will create two processes:
   // 1) It will be the reciever
   // 2) The other process which will create all child processed who will act as the producers

    if (fork() != 0)
    {
        // reciever
        recieve(N, mgsize, outfile);
    }
    else
    {
        // 8 producers
        for (int i = 0; i < N; i++) // loop will run n times
        {
            // create the child process
            int n = fork();

            // only the newly created child process will enter
            if (n == 0)
            {
                producer(i, argv[4 + i], mgsize);
                exit(0);
            }
        }
        for (int i = 0; i < N; i++)
        {
            wait(NULL);
        }
        exit(0);
    }

    wait(NULL);
    // gettimeofday(&afterTime, NULL);
	// printf("***********End of pword************\n");
	// printf("gettimeofday in ms: %ld\n", afterTime.tv_usec);
	// printf("***********************************\n");
	// printf("elapsed time in ms: %ld\n", (afterTime.tv_usec - beforeTime.tv_usec));
	// printf("***********************************\n");
    return 0;
}