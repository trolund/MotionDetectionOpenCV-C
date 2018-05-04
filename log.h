#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>

char buff[255];

char* filename();
char* read();
void addLogging(char* add);
void newFile();
char *time_stamp();
void incrementCounter();

int main() {

    read();
    addLogging(time_stamp());

    return 0;
}

char* filename() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // prefix
    const char *name = "log";

    // get month
    const char * months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    const char* mon = months[tm.tm_mon];

    // get year
    char year[5];
    sprintf(&year, "%d", tm.tm_year + 1900);

    //fileextension
    const char *extension = ".txt";

    char* filename_with_extension;
    filename_with_extension = malloc(strlen(name)+1+4); /* make space for the new string (should check the return value ...) */
    strcpy(filename_with_extension, name); /* copy name into the new var */
    strcat(filename_with_extension, year); /* copy year into the new var */
    strcat(filename_with_extension, mon); /* copy mon into the new var */
    strcat(filename_with_extension, extension); /* add the extension */

    return filename_with_extension;
}

char* read(){

    FILE *fpR = fopen(filename(), "r+");

    if(fpR == NULL)
    {
        printf("Der opstod en fejl. - der bliver oprettet en ny log fil\n");
        do {
            newFile();
            fpR = fopen(filename(), "r+");
        } while (fpR == NULL);
    }
    // hiver data ud af fil
    fscanf(fpR, "%s", buff);
    printf("read from file : %s\n", buff);
    fclose(fpR);
    return buff;
}

void addLogging(char* add){
    FILE *fpAdd = fopen(filename(), "a+");

    incrementCounter();

    printf("%s \n", add);

    size_t len = strlen(add) + strlen("\n");
    char *ret = (char*)calloc(len * sizeof(char) + 1, 1);

    fprintf(fpAdd, "%s", strcat(strcat(ret, add) , "\n"));

    printf("%s \n", add);

    free(ret);

    fclose(fpAdd);
}

void newFile(){
    FILE *fpW = fopen(filename(), "w");

    fseek(fpW, 0, SEEK_SET);

    fprintf(fpW, "0 \n");

    fclose(fpW);

    printf("Ny log fil oprettet.\n");
}

char* time_stamp(){ // modyfied vertion of from https://stackoverflow.com/questions/1444428/time-stamp-in-the-c-programming-language
    char *timestamp = (char *)malloc(sizeof(char) * 16);
    time_t ltime;
    ltime=time(NULL);
    struct tm *tm;
    tm=localtime(&ltime);

    sprintf(timestamp,"%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon,
            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    printf(timestamp,"%04d-%02d-%02d %02d:%02d:%02d \n", tm->tm_year+1900, tm->tm_mon,
           tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    return timestamp;
}

void incrementCounter(){
    FILE *fpR;
    size_t bytesRead;
    char number[5];

    fpR = fopen(filename(), "r+");
    fseek(fpR, 0, SEEK_SET);
    fscanf(fpR,"%20[^\n]\n",number); // kan incrementere tal op til 20 cifre.
    fseek(fpR, 0, SEEK_SET);

    int count = atoi(number);

    count++;

    char str[20];

    sprintf(str, "%d", count);
    // Now str contains the integer as characters

    fprintf(fpR, str);
    fclose(fpR);
}