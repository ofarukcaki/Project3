#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


struct _list {
    char *text;
    int lineNum;
    int upper;  // uppered or not
    int replace;    // replaced or not
    struct list *next;   // next node
};
/* Example list node
 *  {
 *     line: "This is the first line.",
 *     lineNum: 0
 *  }
 */
struct list *root;

char *b = "W0rld";

int lineCount(char *filename) {
    FILE *fp;
    int count = 0;  // Line counter (result)
    char c;
    // Open the file
    fp = fopen(filename, "r");

    // Check if file exists
    if (fp == NULL) {
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

char *getLine(char *file, int lineNum) {

    // TODO: Return the nth line from the file
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    int i = 0;

    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((getline(&line, &len, fp)) != -1) {
        if (i == lineNum) {
            return strdup(line);
        }
        i++;
    }

    fclose(fp);
    if (line)
        free(line);


    fclose(fp);
    return NULL;
}

void *hello(void *arg) {
    printf("Hello %s\n", b);
    return NULL;
}


int main() {
    int readThreadCount = 3;
    int count = lineCount("test.txt");

    printf("Line count: %d\n", count);

    printf("Line %d: %s\n", 0, getLine("test.txt", 0));


    pthread_t newThread;
    pthread_create(&newThread, NULL, &hello, NULL);
    pthread_join(newThread, NULL);
//    sleep(5);
    return 0;
}
