#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// name of the text file we are using
char *myfile;

// hold,ng the values of how many operations to specific thread task is completed
int readCompleted = 0;
int upperCompleted = 0;
int replaceCompleted = 0;
int writeCompleted = 0;

// a structure holding task's operations progresses and results
struct read
{
    char *text;  // line content
    int line;    // line number (which line of the file)
    int upper;   // already uppered or not
    int replace; // already replaced or not
    int busy;    // already taken by upper or replace
};

// a zero fileld array to determine if nth line is already processed by a write thread or not
int writeArray[2000] = {0};

// global records structure, we will use it inside threads in order to access progress of tasks
struct read *records;

// return the total number of of line of a given filename
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

// given a filename and a line number, it returns the text on the file's specific line
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

// given a filename, text to write and wic line to write into.
// this function simply writes text value into file's provided line
// used by writer threads
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

int limit;       // number of linecount of file, also our threshold
int current = 0; // progress indicator for read threads
pthread_mutex_t mutexRead;

// converts given text to uppercase
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

// replaces all spaces with upeprcase in provided string
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
    if (current > limit) // if all lines already given to threads to read, return -1
        return -1;

    rv = current;
    current++; // increment our counter so next thread access to it can get a new line task
    return rv;
}

// a big mutex to lock specific value if it's currently being acceessed by upper thread or replace thread to prevent conflict
// so that a replace thread and uppercase thread will never run on the same arrayvalue of that time
pthread_mutex_t mmm[2000];

// a shared mutex for both upper and replace threads
pthread_mutex_t mutexUpper;

// return which line is going to be converted into uppercase
int getUppercaseIndex()
{
    int i = 0;
    // iterate through all values
    while (i <= limit)
    {
        if (records[i].line != -1 && records[i].upper == 0 && records[i].busy == 0)
        {
            // if the array element is avaialble to take, take it and mark it as busy
            records[i].upper = 1;
            records[i].busy = 1;
            return i;
        }
        i++;
    }
    return -1;
}

// a function which run by uppercase threads,
// it gets which value to convert uppercase from getUppercaseIndex() method and process it
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

// returns which line is going to be replaced with underscores next
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

// a function which run by replace threads
// it gets which value to replaced with underscore from getReplaceIndex() method and process that value
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

// runs by reader threads. read lines from the file and store them inside read struct by their index
void *_read(void *args)
{
    long threadNum = (long)args;
    pthread_mutex_lock(&mutexRead);
    int line = getReadNum(); // request a line index to read;
    pthread_mutex_unlock(&mutexRead);

    // keep reading lines until there will be no lines left to read
    while (line != -1)
    {
        char *text = getLine(myfile, line);
        // fill the default values
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

// returns an available index which is ready to write
// which means a value provessed by both upper thread and replace thread
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

// a mutex lock whichused while writing into file
pthread_mutex_t writeMutex;

// a function run by write threads
// get the write line indexfrom getWriteIndex() and process it
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
    // disable buffering
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

    // set limit variable to line count
    limit = count;

    printf("Line count: %d\n", count);
    fflush(stdout);

    // initialize mutexes
    pthread_mutex_init(&mutexRead, NULL);
    pthread_mutex_init(&mutexUpper, NULL);
    pthread_mutex_init(&writeMutex, NULL);

    // a temporary struct array
    struct read asd[count];
    records = asd;
    // set initial line values to -1
    // which means not processed yet
    for (int i = 0; i < count; i++)
    {
        records[i].line = -1;
    }

    pthread_t readThreads[readThreadCount];

    // CREATE READ THREADS
    for (long i = 0; i < readThreadCount; i++)
    {
        pthread_create(&readThreads[i], NULL, _read, (void *)i);
    }

    pthread_t upperThreads[upperThreadCount];

    // CREATE UPPER THREADS
    for (long i = 0; i < upperThreadCount; i++)
    {
        pthread_create(&upperThreads[i], NULL, upper, (void *)i);
    }

    pthread_t replaceThreads[replaceThreadCount];

    // CREATE REPLACE THREADS
    for (long i = 0; i < replaceThreadCount; i++)
    {
        pthread_create(&replaceThreads[i], NULL, replace, (void *)i);
    }

    pthread_t writeThreads[writeThreadCount];

    // CREATE WRITE THREADS
    for (long i = 0; i < writeThreadCount; i++)
    {
        pthread_create(&writeThreads[i], NULL, _write, (void *)i);
    }

    // WAIT READ THREADS
    for (int i = 0; i < readThreadCount; i++)
    {
        pthread_join(readThreads[i], NULL);
    }
    // WAIT UPPER THREADS
    for (int i = 0; i < upperThreadCount; i++)
    {
        pthread_join(upperThreads[i], NULL);
    }
    // WAIT REPLACE THREADS
    for (int i = 0; i < replaceThreadCount; i++)
    {
        pthread_join(replaceThreads[i], NULL);
    }

    // WAIT WRITE THREADS
    for (int i = 0; i < writeThreadCount; i++)
    {
        pthread_join(writeThreads[i], NULL);
    }

    return 0;
}