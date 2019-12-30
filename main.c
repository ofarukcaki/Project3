#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

char *myfile;

int readCompleted = 0;
int upperCompleted = 0;
int replaceCompleted = 0;
int writeCompleted = 0;

struct read
{
    char *text;
    int line;
    int upper;   // uppered or not
    int replace; // replaced or not
    int busy;
};

int writeArray[200] = {0};

struct read *records;

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
        }
        i++;
    }

    if (line)
        free(line);

    fclose(fp);
    return NULL;
}

void writeToLine(char *filename, char *text, int lineNum)
{
    FILE *fp, *fpTemp;
    char *line = NULL;
    size_t len = 0;
    int i = 0;

    fp = fopen(filename, "r");
    fpTemp = fopen("tempFile123.txt", "a");

    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((getline(&line, &len, fp)) != -1)
    {
        if (i == lineNum)
        {
            fprintf(fpTemp, "%s\n", text);
        }
        else
        {
            fprintf(fpTemp, "%s", line);
            // write original value
        }
        i++;
        fflush(stdout);
        fflush(fpTemp);
    }

    fclose(fpTemp);
    if (line)
        free(line);

    fclose(fp);
    remove(filename);
    rename("tempFile123.txt", filename);
    remove("tempFile123.txt");
}

int limit;
int current = 0;
pthread_mutex_t mutexRead;

char *toUppercase(char *text)
{
    if (text == NULL)
    {
        return NULL;
    }
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
    if (text == NULL)
    {
        return NULL;
    }
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

    rv = current;
    current++;
    return rv;
}
pthread_mutex_t mmm[200];

pthread_mutex_t mutexUpper;

// return which line is going to be converted into uppercase
int getUppercaseIndex()
{
    int i = 0;

    while (i <= limit)
    {
        if (records[i].line != -1 && records[i].upper == 0 && records[i].busy == 0)
        {

            records[i].upper = 1;
            records[i].busy = 1;
            return i;
        }
        i++;
    }
    return -1;
}

void *upper(void *args)
{
    long threadNum = (long)args;
    int line;

    while (1)
    {

        pthread_mutex_lock(&mutexUpper);
        line = getUppercaseIndex();
        pthread_mutex_unlock(&mutexUpper);
        if (line == -1)
        {
            if (readCompleted == 1 && upperCompleted >= (limit + 1))
            {
                break;
            }
            continue;
        }
        pthread_mutex_lock(&mmm[line]);
        char *old = records[line].text;
        records[line].text = toUppercase(old);
        printf("Upper_%d\t\tUpper_%d read index %d and converted \"%s\" to \"%s\"\n", threadNum, threadNum, line, old, records[line].text);
        fflush(stdout);
        records[line].busy = 0;
        upperCompleted++;
        pthread_mutex_unlock(&mmm[line]);
    }
    return NULL;
}

int getReplaceIndex()
{
    int i = 0;

    while (i <= limit)
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
    int line;

    while (1)
    {
        pthread_mutex_lock(&mutexUpper);

        fflush(stdout);

        line = getReplaceIndex();
        pthread_mutex_unlock(&mutexUpper);
        if (line == -1)
        {
            if (readCompleted == 1 && replaceCompleted >= (limit + 1))
                break;
            continue;
        }

        pthread_mutex_lock(&mmm[line]);

        char *old = records[line].text;
        records[line].text = replaceSpace(old);
        printf("Replace_%d\t\tReplace_%d read index %d and converted \"%s\" to \"%s\"\n", threadNum, threadNum, line, old, records[line].text);
        fflush(stdout);
        records[line].busy = 0;
        replaceCompleted++;
        pthread_mutex_unlock(&mmm[line]);
    }

    return NULL;
}

void *tl(void *args)
{
    long threadNum = (long)args;
    pthread_mutex_lock(&mutexRead);
    int line = getReadNum(); // request a line index to read;
    pthread_mutex_unlock(&mutexRead);

    // keep reading lines until there will be no lines left to read
    while (line != -1)
    {
        char *text = getLine(myfile, line);
        records[line].text = text;
        records[line].line = line;
        records[line].upper = 0;
        records[line].replace = 0;
        records[line].busy = 0;

        printf("Read_%d\t\tRead_%d read the line %d which is \"%s\"\n", threadNum, threadNum, line, records[line].text);
        fflush(stdout);

        line = getReadNum();
    }
    readCompleted = 1;
    return NULL;
}

int getWriteIndex()
{
    int i = 0;

    while (i <= limit)
    {
        if (records[i].replace == 1 && records[i].upper == 1 && writeArray[i] == 0)
        {
            writeArray[i] = 1;
            fflush(stdout);
            return i;
        }
        i++;
    }
    return -1;
}

pthread_mutex_t writeMutex;

void *_write(void *args)
{
    long threadNum = (long)args;
    int line;

    while (1)
    {
        pthread_mutex_lock(&writeMutex);
        line = getWriteIndex();
        pthread_mutex_unlock(&writeMutex);

        if (line == -1)
        {
            if (writeCompleted >= (limit + 1))
                break;
            continue;
        }
        pthread_mutex_lock(&writeMutex);

        writeToLine(myfile, records[line].text, records[line].line);
        writeCompleted++;
        pthread_mutex_unlock(&writeMutex);

        printf("Writer_%d\t\tWriter_%d write line %d back which is \"%s\"\n", threadNum, threadNum, records[line].line, records[line].text);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 7)
    {
        printf("Example usage: main.out -d deneme.txt -n 15 5 8 6 \n");
        exit(0);
    }

    myfile = argv[2];
    int count = lineCount(myfile);

    int readThreadCount = atoi(argv[4]);
    int upperThreadCount = atoi(argv[5]);
    int replaceThreadCount = atoi(argv[6]);
    int writeThreadCount = atoi(argv[7]);

    limit = count;

    // disable buffering

    printf("Line count: %d\n", count);
    fflush(stdout);

    // initialize read mutex
    pthread_mutex_init(&mutexRead, NULL);
    pthread_mutex_init(&mutexUpper, NULL);
    pthread_mutex_init(&writeMutex, NULL);

    struct read asd[count];
    records = asd;
    for (int i = 0; i < count; i++)
    {
        records[i].line = -1;
    }

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
        pthread_create(&upperThreads[i], NULL, upper, (void *)i);
    }

    pthread_t replaceThreads[replaceThreadCount];

    // CREATE THREADS
    for (long i = 0; i < replaceThreadCount; i++)
    {
        pthread_create(&replaceThreads[i], NULL, replace, (void *)i);
    }

    pthread_t writeThreads[writeThreadCount];

    // CREATE THREADS
    for (long i = 0; i < writeThreadCount; i++)
    {
        pthread_create(&writeThreads[i], NULL, _write, (void *)i);
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
    for (int i = 0; i < replaceThreadCount; i++)
    {
        pthread_join(replaceThreads[i], NULL);
    }

    // WAIT THREADS
    for (int i = 0; i < writeThreadCount; i++)
    {
        pthread_join(writeThreads[i], NULL);
    }

    return 0;
}