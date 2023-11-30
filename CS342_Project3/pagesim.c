#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 1024

// TWO-LEVEL PAGING
struct level2{
    int arr[1024]; // -2: unavailable, -1: invalid (physical memoryde değil, 0---M ye kadar da  ) 
};

struct level2Node {
    int frameNo;
    int valid; // -1: invalid, 0: valid
} level2Node;

struct level2Node* level1Table[SIZE]; // level 1 table, max 1024 tane level 2 table tutuyor

// LINKED LIST IMPLEMENTATION
struct node
{
    long int base;
    long int end;
    struct node *next;
} node;

void printNodeList(struct node* head) {
    printf("START OF NODE LIST\n");
    struct node* temp = head;
    while(temp != NULL) {
        printf("BASE: %lu and END: %lu \n", temp->base, temp->end);
        temp = temp->next;
    }
    printf("END OF NODE LIST\n\n");
}

void insertNode(struct node **headRef, struct node *head, long int base, long int end)
{

    // insert the first node
    if (head == NULL)
    {
        struct node* newNode = (struct node*) malloc(sizeof(struct node));
        newNode->base = base;
        newNode->end = end;
        newNode->next = NULL;
        *headRef = newNode;
        return;
    }
    else
    {
        struct node* newNode = (struct node*) malloc(sizeof(struct node));
        newNode->base = base;
        newNode->end = end;
        newNode->next = head;
        *headRef = newNode;
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

// QUEUE IMPLEMENTATION
int nodeCount = 0;
int framesUsed = 0;
int maxNodeCount;
struct qNode
{
    long int val;
    struct qNode *next;
};

struct qNode *qHead;
struct qNode *qTail;

struct frameNode
{
    long int frame;
    int counter;
} frameNode;

struct frameNode* lruArray;

void printLRUContent(struct frameNode** arr) {
    printf("\nPRINTING CONTENTS OF LRU ARRAY\n");
    for (int i = 0; i < maxNodeCount; i++)
    {
        if (arr[i] == NULL)
        {
            printf("i: %i empty\n", i);
        }
        else {
            printf("val: %li counter: %i\n", arr[i]->frame, arr[i]->counter);
        }
    }
    printf("END OF CONTENTS OF LRU ARRAY\n");
}

long int insertFrame(struct frameNode** arr, long int frame, int counter)
{
    long int returnVal = frame;
    // if all frames are used find a victim node
    if (framesUsed == maxNodeCount)
    {
        // find the victim node
        int victimIndex = 0;
        int minCounter = arr[0]->counter;
        for (int i = 0; i < maxNodeCount; i++)
        {
            if (arr[i] != NULL && arr[i]->counter < minCounter)
            {
                victimIndex = i;
                minCounter = arr[i]->counter;
            }
        }
        returnVal = arr[victimIndex]->frame;
        // update the victim node with the new node
        arr[victimIndex]->frame = frame;
        arr[victimIndex]->counter = counter;
    }
    // if there is available frames add the new frame
    else
    {
        // create the new frameNode
        struct frameNode* newNode = (struct frameNode*)malloc(sizeof(struct frameNode));
        newNode->counter = counter;
        newNode->frame = frame;
        for (int i = 0; i < maxNodeCount; i++)
        {
            // find an empty spot for the new frameNode
            if (arr[i] == NULL)
            {
                arr[i] = newNode;
                framesUsed = framesUsed + 1;
                break;
            }
        }
        // printf("There is available frame and given frame to %li\n", returnVal);
    }
    return returnVal;
}

void updateClock(struct frameNode** arr, long int frame, int counter) {
    for (int i = 0; i < maxNodeCount; i++) {
        if (arr[i] != NULL && arr[i]->frame == frame) {
            arr[i]->counter = counter;
            // printf("updated counter to %i for frame %li\n", counter, frame);
            return;
        }
    }
}

void printQueue()
{
    printf("START OF PRINT QUEUE\n");
    struct qNode* temp = qHead;
    if (temp == NULL)
        return;
    while (temp != qTail)
    {
        printf("%li ", temp->val);
        temp = temp->next;
    }
    printf("%li ", temp->val);
    printf("\nEND OF PRINT QUEUE\n");
    printf("\n");
}

int addQueue(int n)
{
    if (nodeCount == maxNodeCount)
        return -1;

    struct qNode *newNode = (struct qNode *)malloc(sizeof(struct qNode));

    newNode->val = n;
    newNode->next = NULL;

    if (qHead == NULL)
    {
        qHead = newNode;
        qTail = newNode;
    }
    else
    {
        qTail->next = newNode;
        qTail = qTail->next;
    }

    nodeCount++;
    return 0;
}

int popQueue()
{
    if (nodeCount == 0)
        return -1;

    struct qNode *res = qHead;
    qHead = qHead->next;
    int resVal = res->val;
    res->next = NULL;
    free(res);
    nodeCount--;
    return resVal;
}

void deleteQueue() {
    struct qNode* temp;
    while(qHead != NULL) {
        temp = qHead;
        qHead = qHead->next;
        free(temp);
    }
    qHead = NULL;
    qTail = NULL;
}

long int power(int a, int n) {
    long int res = 1;
    for(int i = 0; i < n; i++) 
        res *= a;
    return res;
}

char* subString(char* str, char* res, int begin, int stop) {  
    for(int i = begin; i < stop; i++ ) {;
        res[i - begin] = str[i];
    }
    return res;
}

char* hexToBin(char* hex, char* bin) {
    for(int i = 0; i < 8; i++) {
        if(hex[i+2] == '0')
            strcat(bin, "0000");
        if(hex[i+2] == '1')
            strcat(bin, "0001");
        if(hex[i+2] == '2')
            strcat(bin, "0010");
        if(hex[i+2] == '3')
            strcat(bin, "0011");
        if(hex[i+2] == '4')
            strcat(bin, "0100");
        if(hex[i+2] == '5')
            strcat(bin, "0101");
        if(hex[i+2] == '6')
            strcat(bin, "0110");
        if(hex[i+2] == '7')
            strcat(bin, "0111");
        if(hex[i+2] == '8')
            strcat(bin, "1000");
        if(hex[i+2] == '9')
            strcat(bin, "1001");
        if(hex[i+2] == 'a' || hex[i+2] == 'A')
            strcat(bin, "1010");
        if(hex[i+2] == 'b' || hex[i+2] == 'B')
            strcat(bin, "1011");
        if(hex[i+2] == 'c' || hex[i+2] == 'C')
            strcat(bin, "1100");
        if(hex[i+2] == 'd'|| hex[i+2] == 'D')
            strcat(bin, "1101");
        if(hex[i+2] == 'e' || hex[i+2] == 'E')
            strcat(bin, "1110");
        if(hex[i+2] == 'f' || hex[i+2] == 'F')
            strcat(bin, "1111");
    }
    return bin;
}

long int binToDec(char* bin) {
    long int dec = 0;
    int p = strlen(bin);
    for (int i = 0; i < strlen(bin); i++) {
        dec += (bin[i] - '0') * power(2, p - 1);
        p--;
    }
    return dec;
}

void decToBin(long int n, char* result)
{
    char binaryNum[20];
    for (int i = 0; i < 20; i++) {
        binaryNum[i] = '0';
    }

    int i = 0;
    while (n > 0) {
        binaryNum[i] = (n % 2) + '0';
        n = n / 2;
        i++;
    }

    // printf("binary num: %s\n", binaryNum);

    // reverse order to inorder
    int k = 0;
    for(int i = 19; i >= 0; i--) {
        result[k] = binaryNum[i];
        k++;
    }
}

long int getOuterAddress(char* bin) {
    // printf("input for (outer address): %s\n", bin);
    char temp[10];
    for (int i = 0; i < 10; i++) {
        temp[i] = bin[i];
    }
    // printf("temp (outer address): %s\n", temp);
    long int dec = 0;
    int p = strlen(temp);
    for (int i = 0; i < strlen(temp); i++) {
        dec += (temp[i] - '0') * power(2, p - 1);
        p--;
    }
    return dec;
}

long int getInnerAddress(char* bin) {
    char temp[10];
    for (int i = 0; i < 10; i++) {
        temp[i] = bin[i + 10];
    }
    // printf("temp: %s\n", temp);
    long int dec = 0;
    int p = strlen(temp);
    for (int i = 0; i < strlen(temp); i++) {
        dec += (temp[i] - '0') * power(2, p - 1);
        p--;
    }
    return dec;
}

long int getOffset(char* bin) {
    char temp[12];
    for (int i = 0; i < 12; i++) {
        temp[i] = bin[i + 20];
    }
    // printf("temp: %s\n", temp);
    long int dec = 0;
    int p = strlen(temp);
    for (int i = 0; i < strlen(temp); i++) {
        dec += (temp[i] - '0') * power(2, p - 1);
        p--;
    }
    return dec;
}

long int getValue(char* bin) {
    char* temp = (char*)malloc(20);
    // printf("temp (before): %s\n", temp);
    for (int i = 0; i < 20; i++) {
        temp[i] = bin[i];
        // printf("i = %i temp: %c\n", i, bin[i]);
        // printf("temp (growing): %li\n", strlen(temp));
    }
    // printf("temp (after): %s\n", temp);
    long int dec = 0;
    int p = strlen(temp);
    for (int i = 0; i < strlen(temp); i++) {
        dec += (temp[i] - '0') * power(2, p - 1);
        p--;
    }
    free(temp);
    return dec;
}

// MAIN
int main(int argc, char *argv[]) {
    char *infile1, *infile2, *outfile;
    char *VMSsize;
    int M, alg, addressCount;
    int MODE = 1;

    if (argc == 7) {
        MODE = 1;
        infile1 = argv[1];
        infile2 = argv[2];
        M = atoi(argv[3]);
        maxNodeCount = M;
        outfile = argv[4];
        alg = atoi(argv[6]);
    }
    else if (argc == 9) {
        MODE = 2;
        M = atoi(argv[1]);
        outfile = argv[2];
        alg = atoi(argv[4]);
        VMSsize = argv[6];
        addressCount = atoi(argv[8]);
        maxNodeCount = M;
    }
    else { 
        printf("Wrong input format! \n"); 
        return -1; 
    }
    FILE* outptr;
    outptr = fopen(outfile, "w");
    for (int i = 0; i < SIZE; i++)
        level1Table[i] = NULL;    

    int frames[M];
    for(int i = 0; i < M; i++) {
        frames[i] = 0; // physical Memory is empty initially
    }
    struct frameNode* lruArray[maxNodeCount];
    for(int i = 0; i < M; i++) {
        lruArray[i] = NULL; // physical Memory is empty initially
    }
    struct node *rangeList = NULL;
    if (MODE == 1) {
        // READ FILE 1
        FILE *fp; 
        fp = fopen(infile1, "r");
        char readed[10];

        if (fp == NULL) { 
            perror("File could not be opened\n"); 
            exit(1);
        } 
        else {
            while (fscanf(fp, "%s", readed) != EOF) {
                char *ptr;
                long int base = strtoul(readed, &ptr, 16);
                fscanf(fp, "%s", readed);
                long int end = strtoul(readed, &ptr, 16);
                insertNode(&rangeList, rangeList, base, end);
            }
            fclose(fp);
        }
        FILE *fp2;
        fp2 = fopen(infile2, "r");
        if (fp2 == NULL) { 
            perror("Can't open file2\n"); 
            exit(1);
        }
        int timeCounter = 0;
        while (fscanf(fp2, " %s", readed) != EOF) {
            char *ptr;
            long int val = strtoul(readed, &ptr, 16);
            int inRange = 0;
            struct node* temp = rangeList;
            while(temp != NULL) {
                if(val < temp->end && val >= temp->base)
                    inRange = 1;
                temp = temp->next;
            }
            if(!inRange) {
                // printf("%s e\n", readed);
                fprintf(outptr, "%s e\n", readed);
                continue;
            }
            char *binReaded = (char *)malloc(32);
            hexToBin(readed, binReaded);
            long int outerAddress = getOuterAddress(binReaded);
            long int innerAddress = getInnerAddress(binReaded);
            long int value = getValue(binReaded);
            if(level1Table[outerAddress] == NULL){
                struct level2Node* newLevel2 = malloc(SIZE * sizeof(level2Node));
                level1Table[outerAddress] = newLevel2;
                for(int i = 0; i < SIZE; i++) {
                    newLevel2[i].frameNo = -1; // no frames assigned
                    newLevel2[i].valid = -1; // not in physical memory, so invalid
                }
            }
            int frameToGive;
            if (level1Table[outerAddress][innerAddress].valid == -1) {
                 
                if (alg == 1) {
                    if (nodeCount == maxNodeCount) {
                        long int victimValue = popQueue();
                        char binVictimValue[20];
                        decToBin(victimValue, binVictimValue);
                        long int oaVictim = getOuterAddress(binVictimValue);
                        long int iaVictim = getInnerAddress(binVictimValue);
                        level1Table[oaVictim][iaVictim].valid = -1;
                        frameToGive = level1Table[oaVictim][iaVictim].frameNo;
                    }
                    else {
                        for (int i = 0; i < maxNodeCount; i++) {
                            if (frames[i] == 0) {
                                frameToGive = i;
                                break;
                            }
                        }
                    }
                    addQueue(value);
                }
                if (alg == 2) {
                    if (framesUsed == maxNodeCount) {
                        long int victimValue = insertFrame(lruArray, value, timeCounter);
                        char binVictimValue[20];
                        decToBin(victimValue, binVictimValue);
                        long int oaVictim = getOuterAddress(binVictimValue);
                        long int iaVictim = getInnerAddress(binVictimValue);
                        level1Table[oaVictim][iaVictim].valid = -1;
                        frameToGive = level1Table[oaVictim][iaVictim].frameNo;
                    }
                    else {
                        for (int i = 0; i < maxNodeCount; i++) {
                            if (frames[i] == 0) {
                                frameToGive = i;
                                break;
                            }
                        }
                        insertFrame(lruArray, value, timeCounter);
                    }
                }

               
                level1Table[outerAddress][innerAddress].valid = 1;
                frames[frameToGive] = 1;
                level1Table[outerAddress][innerAddress].frameNo = frameToGive;
                int frameDecimal = frameToGive;
                char binNumTemp[20];
                for (int i = 0; i < 20; i++) {
                    binNumTemp[i] = '0';
                }

                char binNumRes[20];
                for (int i = 0; i < 20; i++) {
                    binNumRes[i] = '0';
                }

                int i = 0;
                while (frameDecimal > 0) {
                    binNumTemp[i] = (frameDecimal % 2) + '0';
                    frameDecimal = frameDecimal / 2;
                    i++;
                }

                // reverse order to inorder
                int k = 0;
                for(int i = 19; i >= 0; i--) {
                    binNumRes[k] = binNumTemp[i];
                    k++;
                }
                char* physicallAddressBin = (char*)malloc(32);
                for (int i = 0; i < 20; i++) {
                    physicallAddressBin[i] = binNumRes[i];
                }
                for (int i = 20; i < 32; i++) {
                    physicallAddressBin[i] = binReaded[i];
                }
                int physicallAddressDecimal = (int)strtol(physicallAddressBin, NULL, 2);
                char pyhsicalAddressHex[32]; 
                sprintf(pyhsicalAddressHex, "0x%08x", physicallAddressDecimal); 
                // printf( "%s x\n",  pyhsicalAddressHex); 
                fprintf(outptr, "%s x\n",  pyhsicalAddressHex);
            }
            else {
                int frameDecimal = level1Table[outerAddress][innerAddress].frameNo;
                char binNumTemp[20];
                for (int i = 0; i < 20; i++) {
                    binNumTemp[i] = '0';
                }

                char binNumRes[20];
                for (int i = 0; i < 20; i++) {
                    binNumRes[i] = '0';
                }

                int i = 0;
                while (frameDecimal > 0) {
                    binNumTemp[i] = (frameDecimal % 2) + '0';
                    frameDecimal = frameDecimal / 2;
                    i++;
                }

                // reverse order to inorder
                int k = 0;
                for(int i = 19; i >= 0; i--) {
                    binNumRes[k] = binNumTemp[i];
                    k++;
                }
                
                // binNumRes should be concatted with the offset and then converted to hex
                char* physicallAddressBin = (char*)malloc(32);
                //char physicallAddressBin[32];
                for (int i = 0; i < 20; i++) {
                    physicallAddressBin[i] = binNumRes[i];
                }
                for (int i = 20; i < 32; i++) {
                    physicallAddressBin[i] = binReaded[i];
                }

                int physicallAddressDecimal = (int)strtol(physicallAddressBin, NULL, 2);
                char pyhsicalAddressHex[32]; 
                sprintf(pyhsicalAddressHex, "0x%08x", physicallAddressDecimal); 
                
                // printf("%s \n",  pyhsicalAddressHex); 
                fprintf(outptr, "%s \n",  pyhsicalAddressHex); 
  
                updateClock(lruArray, value, timeCounter);
            }
            timeCounter = timeCounter + 1;
        }
    }
    if (MODE == 2) {
        char *ptr;
        long int end = strtoul(VMSsize, &ptr, 16);
        insertNode(&rangeList, rangeList, 0, end);
        char *binReaded = (char *)malloc(32);
        hexToBin(VMSsize, binReaded);
        insertNode(&rangeList, rangeList, 0, end);
        for (int timeCounter = 0; timeCounter < addressCount; timeCounter++) {
            long int val = rand() % end;
            char* readed = (char*)malloc(32);
            sprintf(readed, "0x%08x", val); 
            // printf("%s ", readed);
            fprintf(outptr, "%s ", readed);
            char *binReaded = (char *)malloc(32);
            hexToBin(readed, binReaded);
            long int outerAddress = getOuterAddress(binReaded);
            long int innerAddress = getInnerAddress(binReaded);
            long int value = getValue(binReaded);
            if(level1Table[outerAddress] == NULL) {
                struct level2Node* newLevel2 = malloc(SIZE * sizeof(level2Node));
                level1Table[outerAddress] = newLevel2;
                for(int i = 0; i < SIZE; i++) {
                    newLevel2[i].frameNo = -1; // no frames assigned
                    newLevel2[i].valid = -1; // not in physical memory, so invalid
                }
            }
            int frameToGive;
            
            if (level1Table[outerAddress][innerAddress].valid == -1) {
                 
                if (alg == 1) {
                    if (nodeCount == maxNodeCount) {
                        long int victimValue = popQueue();
                        char binVictimValue[20];
                        decToBin(victimValue, binVictimValue);
                        long int oaVictim = getOuterAddress(binVictimValue);
                        long int iaVictim = getInnerAddress(binVictimValue);
                        level1Table[oaVictim][iaVictim].valid = -1;
                        frameToGive = level1Table[oaVictim][iaVictim].frameNo;
                    }
                    else {
                        for (int i = 0; i < maxNodeCount; i++) {
                            if (frames[i] == 0) {
                                frameToGive = i;
                                break;
                            }
                        }
                    }
                    addQueue(value);
                }
                if (alg == 2) {
                    if (framesUsed == maxNodeCount) {
                        long int victimValue = insertFrame(lruArray, value, timeCounter);
                        char binVictimValue[20];
                        decToBin(victimValue, binVictimValue);
                        long int oaVictim = getOuterAddress(binVictimValue);
                        long int iaVictim = getInnerAddress(binVictimValue);
                        level1Table[oaVictim][iaVictim].valid = -1;
                        frameToGive = level1Table[oaVictim][iaVictim].frameNo;
                    }
                    else {
                        for (int i = 0; i < maxNodeCount; i++) {
                            if (frames[i] == 0) {
                                frameToGive = i;
                                break;
                            }
                        }
                        insertFrame(lruArray, value, timeCounter);
                    }
                }

               
                level1Table[outerAddress][innerAddress].valid = 1;
                frames[frameToGive] = 1;
                level1Table[outerAddress][innerAddress].frameNo = frameToGive;
                int frameDecimal = frameToGive;
                char binNumTemp[20];
                for (int i = 0; i < 20; i++) {
                    binNumTemp[i] = '0';
                }

                char binNumRes[20];
                for (int i = 0; i < 20; i++) {
                    binNumRes[i] = '0';
                }

                int i = 0;
                while (frameDecimal > 0) {
                    binNumTemp[i] = (frameDecimal % 2) + '0';
                    frameDecimal = frameDecimal / 2;
                    i++;
                }

                // reverse order to inorder
                int k = 0;
                for(int i = 19; i >= 0; i--) {
                    binNumRes[k] = binNumTemp[i];
                    k++;
                }
                char* physicallAddressBin = (char*)malloc(32);
                for (int i = 0; i < 20; i++) {
                    physicallAddressBin[i] = binNumRes[i];
                }
                for (int i = 20; i < 32; i++) {
                    physicallAddressBin[i] = binReaded[i];
                }
                int physicallAddressDecimal = (int)strtol(physicallAddressBin, NULL, 2);
                char pyhsicalAddressHex[32]; 
                sprintf(pyhsicalAddressHex, "0x%08x", physicallAddressDecimal); 
                // printf( "%s x\n",  pyhsicalAddressHex); 
                fprintf(outptr, "%s x\n",  pyhsicalAddressHex);
            }
            else {
                // FRAME IS ALREADY AVAILABLE. THUS PRINT ONLY THE PHYSICALL ADRESS
                int frameDecimal = level1Table[outerAddress][innerAddress].frameNo;
                char binNumTemp[20];
                for (int i = 0; i < 20; i++) {
                    binNumTemp[i] = '0';
                }

                char binNumRes[20];
                for (int i = 0; i < 20; i++) {
                    binNumRes[i] = '0';
                }

                int i = 0;
                while (frameDecimal > 0) {
                    binNumTemp[i] = (frameDecimal % 2) + '0';
                    frameDecimal = frameDecimal / 2;
                    i++;
                }

                // reverse order to inorder
                int k = 0;
                for(int i = 19; i >= 0; i--) {
                    binNumRes[k] = binNumTemp[i];
                    k++;
                }
                // binNumRes should be concatted with the offset and then converted to hex
                char* physicallAddressBin = (char*)malloc(32);
                //char physicallAddressBin[32];
                for (int i = 0; i < 20; i++) {
                    physicallAddressBin[i] = binNumRes[i];
                }
                for (int i = 20; i < 32; i++) {
                    physicallAddressBin[i] = binReaded[i];
                }
                // convert binary string to integer
                int physicallAddressDecimal = (int)strtol(physicallAddressBin, NULL, 2);
                char pyhsicalAddressHex[32]; 
                sprintf(pyhsicalAddressHex, "0x%08x", physicallAddressDecimal); 
                // printf( "%s \n",  pyhsicalAddressHex);
                fprintf(outptr, "%s \n",  pyhsicalAddressHex);
                updateClock(lruArray, value, timeCounter);
            }
        }
    }
    fclose(outptr);
    deleteList(rangeList);
    deleteQueue();
    return 0;
}