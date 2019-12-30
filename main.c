#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

struct read
{
    char *text;
    int line;
    int upper;   // uppered or not
    int replace; // replaced or not
    int busy;
};

struct read *records;

/* Example list node
 *  {
 *     line: "This is the first line.",
 *     lineNum: 0
 *  }
 */

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

int limit;
int current = 0;
pthread_mutex_t mutexRead;

char *toUppercase(char *text)
{
    int i = 0;
    char *str = strdup(text);

    while (str[i])
    {
        if (str[i] >= 97 && str[i] <= 122)
            str[i] -= 32;
        i++;
    }
    return (str);
}

char *replaceSpace(char *text)
{
    int i = 0;
    char *str = strdup(text);

    while (str[i])
    {
        if (str[i] == ' ')
            str[i] = '_';
        i++;
    }
    return (str);
}

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

pthread_mutex_t mutexUpper;

// return which line is going to be converted into uppercase
int getUppercaseIndex()
{
    // pthread_mutex_lock(&mutexUpper);
    int i = 0;

    while (i < limit)
    {
        if (records[i].line != -1 && records[i].upper == 0 && records[i].busy == 0)
        {

            records[i].upper = 1;
            records[i].busy = 1;
            return i;
        }
        i++;
    }
    // pthread_mutex_unlock(&mutexUpper);
    return -1;
}

void *upper(void *args)
{
    long threadNum = (long)args;

    pthread_mutex_lock(&mutexUpper);
    int line = getUppercaseIndex();
    pthread_mutex_unlock(&mutexUpper);

    while (line != -1)
    {
        char *old = records[line].text;
        records[line].text = toUppercase(old);
        printf("Upper_%d\t\tUpper_%d read index %d and converted \"%s\" to \"%s\"\n", threadNum, threadNum, line, old, records[line].text);
        records[line].busy = 0;
        pthread_mutex_lock(&mutexUpper);
        line = getUppercaseIndex();
        pthread_mutex_unlock(&mutexUpper);
    }
    return NULL;
}

int getReplaceIndex()
{
    int i = 0;

    while (i < limit)
    {
        if (records[i].line != -1 && records[i].replace == 0 && records[i].busy == 0)
        {

            records[i].replace = 1;
            records[i].busy = 1;
            return i;
        }
        i++;
    }
    return -1;
}

void *replace(void *args)
{
    long threadNum = (long)args;

    pthread_mutex_lock(&mutexUpper);
    int line = getReplaceIndex();
    pthread_mutex_unlock(&mutexUpper);

    while (line != -1)
    {
        char *old = records[line].text;
        records[line].text = replaceSpace(old);
        printf("Replace_%d\t\tReplace_%d read index %d and converted \"%s\" to \"%s\"\n", threadNum, threadNum, line, old, records[line].text);
        records[line].busy = 0;
        pthread_mutex_lock(&mutexUpper);
        line = getReplaceIndex();
        pthread_mutex_unlock(&mutexUpper);
    }
    return NULL;
}

void *tl(void *args)
{
    long threadNum = (long)args;
    int line = getReadNum(); // request a line index to read;
    // keep reading lines until there will be no lines left to read
    while (line != -1)
    {
        char *text = getLine("test.txt", line);
        records[line].text = text;
        records[line].line = line;
        records[line].upper = 0;
        records[line].replace = 0;
        records[line].busy = 0;

        // printf("> : _%s(%d)_ Thread: %ld\n", records[line].text, records[line].line, threadNum);
        printf("Read_%d\t\tRead_%d read the line %d which is \"%s\"\n", threadNum, threadNum, line, records[line].text);
        // fflush(stdout);

        // if (records[7].line != -1)
        //     printf("7 is null\n");

        // printf("line num: %d\n", line);
        line = getReadNum();
    }
    return NULL;
}

int main()
{
    int readThreadCount = 5;
    int upperThreadCount = 3;
    int replaceThreadCount = 3;
    int count = lineCount("test.txt");
    limit = 15;
    // limit = count;

    // disable buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Line count: %d\n", count);
    // printf("Replace: *%s*\n", replaceSpace("hello bitch w0rld ^^"));

    // initialize read mutex
    pthread_mutex_init(&mutexRead, NULL);
    pthread_mutex_init(&mutexUpper, NULL);

    // printf("GetReadnum: %d\n", getReadNum());

    struct read asd[count];
    records = asd;
    for (int i = 0; i < count; i++)
    {
        records[i].line = -1;
    }

    // records[0].line = 5;
    // records[1].line = 6;

    // printf("0:%d\n",records[0].line);
    // printf("1:%d\n",records[1].line);

    pthread_t readThreads[readThreadCount];

    // CREATE THREADS
    for (long i = 0; i < readThreadCount; i++)
    {
        pthread_create(&readThreads[i], NULL, tl, (void *)i);
    }

    pthread_t upperThreads[upperThreadCount];

    // CREATE THREADS
    for (long i = 0; i < upperThreadCount; i++)
    {
        // printf("upper thread %d started\n", i);
        pthread_create(&upperThreads[i], NULL, upper, (void *)i);
    }

    pthread_t replaceThreads[replaceThreadCount];

    // CREATE THREADS
    for (long i = 0; i < replaceThreadCount; i++)
    {
        // printf("upper thread %d started\n", i);
        pthread_create(&replaceThreads[i], NULL, replace, (void *)i);
    }

    // WAIT THREADS
    for (int i = 0; i < readThreadCount; i++)
    {
        pthread_join(readThreads[i], NULL);
    }
    // WAIT THREADS
    for (int i = 0; i < upperThreadCount; i++)
    {
        pthread_join(upperThreads[i], NULL);
    }
    // WAIT THREADS
    // for (int i = 0; i < replaceThreadCount; i++)
    // {
    //     pthread_join(replaceThreads[i], NULL);
    // }

    // pthread_t newThread;
    // pthread_create(&newThread, NULL, &tl, NULL);

    // pthread_join(newThread, NULL);
    //    sleep(5);
    return 0;
}
