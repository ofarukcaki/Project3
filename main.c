#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

struct _list
{
    char *text;
    int lineNum;
    int upper;         // uppered or not
    int replace;       // replaced or not
    struct list *next; // next node
};
/* Example list node
 *  {
 *     line: "This is the first line.",
 *     lineNum: 0
 *  }
 */
struct list *root;

char *b = "W0rld";

int lineCount(char *filename)
{
    FILE *fp;
    int count = 0; // Line counter (result)
    char c;
    // Open the file
    fp = fopen(filename, "r");

    // Check if file exists
    if (fp == NULL)
    {
        printf("Could not open file %s", filename);
        return 0;
    }

    // Extract characters from file and store in character c
    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            count = count + 1;

    // Close the file
    fclose(fp);
    return count;
}

char *getLine(char *file, int lineNum)
{

    // TODO: Return the nth line from the file
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    int i = 0;

    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((getline(&line, &len, fp)) != -1)
    {
        if (i == lineNum)
        {
            // remove line ending character LF
            return strndup(line, strlen(line) - 1);
            // return strdup(line);
        }
        i++;
    }

    fclose(fp);
    if (line)
        free(line);

    fclose(fp);
    return NULL;
}

void *hello(void *arg)
{
    printf("Hello %s\n", b);
    return NULL;
}

int limit = 10;
int current = 0;
pthread_mutex_t mutexRead;

// determine the line to be read by a thread
int getReadNum()
{
    int rv;
    if (current > limit)
        return -1;
    pthread_mutex_lock(&mutexRead);

    rv = current;
    current++;
    pthread_mutex_unlock(&mutexRead);
    // printf("%d returned\n", rv);
    return rv;
}

void *tl(void *args)
{
    long threadNum = (long)args;
    int line = getReadNum(); // request a line index to read;
    // keep reading lines until there will be no lines left to read
    while (line != -1)
    {
        char * text = getLine("test.txt", line);
        /*
            create a struct and keep its address into array
            when check needed go to that adress and inspect the struct        
        */
        printf("> : _%s_ Thread: %ld\n",text , threadNum);
        // printf("line num: %d\n", line);
        line = getReadNum();
    }
}

int main()
{
    int readThreadCount = 10;
    int count = lineCount("test.txt");

    printf("Line count: %d\n", count);

    // initialize read mutex
    pthread_mutex_init(&mutexRead, NULL);

    // printf("GetReadnum: %d\n", getReadNum());

    pthread_t readThreads[readThreadCount];

    // CREATE THREADS
    for (long i = 0; i < readThreadCount; i++)
    {
        pthread_create(&readThreads[i], NULL, tl, (void *)i);
    }

    // WAIT THREADS
    for (int i = 0; i < readThreadCount; i++)
    {
        pthread_join(readThreads[i], NULL);
    }

    // pthread_t newThread;
    // pthread_create(&newThread, NULL, &tl, NULL);

    // pthread_join(newThread, NULL);
    //    sleep(5);
    return 0;
}
