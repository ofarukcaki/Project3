#include <stdio.h>


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
    FILE *f = fopen(file, "r");
    // TODO: Return the nth line from the file
    fclose(file);
}

int main() {
    int count = lineCount("test.txt");

    printf("Line count: %d\n", count);

//    getLine("test.txt",2);

    return 0;
}
