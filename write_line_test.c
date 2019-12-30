#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void writeToLine(char *filename, char *text, int lineNum)
{
    // TODO: Return the nth line from the file
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
            // remove line ending character LF
            fprintf(fpTemp, "%s", line);
            // return strndup(line, strlen(line) - 1);
            // write original value
        }
        i++;
    }

    fclose(fpTemp);
    if (line)
        free(line);

    fclose(fp);
    remove(filename);
    rename("tempFile123.txt", filename);
    remove("tempFile123.txt");
}

void writeTest()
{
    FILE *fptr;
    // opening file in writing mode
    fptr = fopen("p.txt", "a");
    // exiting program
    if (fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }
    fprintf(fptr, "%s", "hello im trump\n");
    fclose(fptr);
}

int main()
{

    // writeTest();
    writeToLine("test.txt", "asd", 3);

    return 0;
}