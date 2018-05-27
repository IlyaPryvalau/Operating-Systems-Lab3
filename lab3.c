#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include "inolist.h"
#include "periodlist.h"
#define BUFFER_SIZE 1024 * 1024 * 16
#define ERROR_BUFFER_SIZE 1024 * 8192
#define OUTPUT_BUFFER 2 * 8192
#define BITS_IN_BYTE 8
#define NOT_SET 2
#define ERROR_FILE "/tmp/error_log.txt"

dev_t rootDev;
TInoList IList;
int errBuffLength, FCurr, bufferOverflow, maxProc, runningProc;
char errLine[8192], *errorBuffer;


//function prototypes definitions
void BufferedWrite(int fd, char *buffer, char *errorLine);
void DirExplore(char *programName, char *dirName, int errorLog, int *maxProc, int *runningProc);
void PrintError(int fd);
int BitCount(int errorLog, char *filePath, char *programName);
int CreateProcess(int errorLog, char *filePath, char *programName, int *maxProc, int *runningProc);
//function prototypes definitions end

void main(int argc, char *argv[]) {
    int FErrorLog, maxProc;
    struct stat entryStatRoot;
    unsigned char *programName = basename(argv[0]), *dirName;
    IList = InoListInit(&IList);
    errorBuffer = (char *)malloc(ERROR_BUFFER_SIZE);
    errBuffLength = 0;
    bufferOverflow = 0;
    runningProc = 0;

    if ((FErrorLog = open(ERROR_FILE, O_RDWR | O_CREAT, S_IRWXU)) == -1) {
        printf("%s: unable create error file at %s %s\n", programName, ERROR_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (argc != 3) {
        printf("%s: incorrect amount of parameters (3 needed)\ncorrect usage: %s [name_of_directory] [amount_of_Processes]\n",
        programName, programName);
        exit(EXIT_FAILURE);
    }
    if ((dirName = realpath(argv[1], NULL)) == NULL) {
        printf("%s: %s %s\n", programName, argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }
    maxProc = atoi(argv[2]);
    if (lstat(dirName, &entryStatRoot) != 0) {
        printf("%s: %s %s\n", programName, argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }
    else
        rootDev = entryStatRoot.st_dev;
    bufferOverflow = 0;
    DirExplore(programName, dirName, FErrorLog, &maxProc, &runningProc);
    //printf("%s: %d %s\n",programName, getpid(), dirName); //uncomment if you want to see directories
    if (bufferOverflow)
        write(FErrorLog, errorBuffer, strlen(errorBuffer));
    lseek(FErrorLog, 0, SEEK_SET);
    int status;
    while (runningProc > 0) {
        wait(&status);
        runningProc--;
    }
    PrintError(FErrorLog);
    free(IList.pointer);
    close(FErrorLog);
    remove(ERROR_FILE);
}

void BufferedWrite(int fd, char *buffer, char *errorLine){
    errBuffLength += strlen(errorLine);
    if (errBuffLength < ERROR_BUFFER_SIZE){
        strcat(buffer, errLine);
    } else {
        bufferOverflow = 1;
        write(fd, buffer, strlen(buffer));
        buffer[0] = '\0';
        strcat(buffer, errLine);     
        errBuffLength = strlen(buffer);   
    }
}

void PrintError(int fd){
    if (bufferOverflow) {
        struct stat st;
        lstat(ERROR_FILE, &st);
        for (int i = 0; i < st.st_size / ERROR_BUFFER_SIZE; i++){
            read(fd, errorBuffer, ERROR_BUFFER_SIZE);
            fputs(errorBuffer, stdout);        
        }
        int bufferTailLength = st.st_size % ERROR_BUFFER_SIZE;
        read(fd, errorBuffer, bufferTailLength);
        errorBuffer[bufferTailLength] = '\0';
        fputs(errorBuffer, stdout);
    } else {
        fputs(errorBuffer, stdout);
    }
}

void DirExplore(char *programName, char *dirName, int errorLog, int *maxProc, int *runningProc) {
    DIR *dir;
    struct dirent *dirEntry;
    struct stat dirEntryStat;
    char *entryPath = NULL;

    if ((dir = opendir(dirName)) != NULL){
        while ((dirEntry = readdir(dir)) != NULL) {
            //forming the dir entry absolute path
            entryPath = (char *)malloc((strlen(dirName) + strlen(dirEntry->d_name) + 2) * sizeof(char));
            strcpy(entryPath, dirName);
            if (dirName[strlen(dirName) - 1] != '/')
                strcat(entryPath, "/");
            strcat(entryPath, dirEntry->d_name);
            if (lstat(entryPath, &dirEntryStat) == 0) {
                if (dirEntryStat.st_dev == rootDev){
                    //if file is a directory
                    if (dirEntry->d_type == DT_DIR) {
                        if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
                            continue;
                        else {
                            DirExplore(programName, entryPath, errorLog, maxProc, runningProc);
                            //printf("%s: %d %s\n",programName, getpid(), entryPath); //uncomment if you want to see directories
                        }
                    }
                    //if file is a regular file
                    if (dirEntry->d_type == DT_REG) {
                        //if multiple hard links
                        if (dirEntryStat.st_nlink > 1) {
                            if (InoInList(&IList, dirEntryStat.st_ino, InoCompareInList) == -1) {
                                InoListAdd(&IList, dirEntryStat.st_ino);
                                //printf("%s: %d %s",programName, getpid(), entryPath);
                                CreateProcess(errorLog, entryPath, programName, maxProc, runningProc);
                            }
                        } else {
                            //printf("%s: %d %s",programName, getpid(), entryPath);
                            CreateProcess(errorLog, entryPath, programName, maxProc, runningProc); 
                        }
                    }
                    //if file is a sym link
                    if (dirEntry->d_type == DT_LNK) {
                        printf("%d %s (sym)\n", getpid(), entryPath);
                    }
                } 
            } else {
                snprintf(errLine, 8192, "%d %s %s\n", getpid(), entryPath, strerror(errno));
                BufferedWrite(errorLog, errorBuffer, errLine); 
            }
            free(entryPath);
        }
        closedir(dir);
    } else {
        snprintf(errLine, 8192, "%d %s %s\n", getpid(), dirName, strerror(errno));
        BufferedWrite(errorLog, errorBuffer, errLine);
    }
}

int BitCount(int errorLog, char *filePath, char *programName) {

    unsigned long long bytesInFile, bytesCounted, bytesReadTotal;
    unsigned char *buffer = (unsigned char*)malloc(BUFFER_SIZE), byte[8];
    int bytesRead;
    struct stat statStruct;
    TPeriodList PList, ListArr[2];
    ListArr[0] = ListArr[1] = PeriodListInit(&PList);
    T_PERIOD tempPeriod;
    tempPeriod.count = 0;
    tempPeriod.period = 0;
    char lastBit = NOT_SET;

    lstat(filePath, &statStruct);
    bytesInFile = (unsigned long long)statStruct.st_size; 
    bytesRead = bytesReadTotal = read(FCurr, buffer, BUFFER_SIZE); 
    int CountPeriods(char startIndex){
        int ind, i = startIndex;
        void AddPeriod(char index) {
            if (ListArr[index].pointer == NULL || (ind = PeriodInList(&ListArr[index], tempPeriod, PeriodCompareInList)) == -1){
                PeriodListAdd(&ListArr[index], tempPeriod);
                ListArr[index].pointer[ListArr[index].itemCount - 1].count++;
            }
            else   
                ListArr[index].pointer[ind].count++;
            tempPeriod.period = 0;
        }

        if (lastBit != NOT_SET && bytesCounted < bytesInFile)
            if (lastBit == byte[startIndex]){
                tempPeriod.period++;
                i++;
                lastBit = NOT_SET;
            } else {
                AddPeriod(lastBit);
                lastBit = NOT_SET;
                return startIndex;
            }
        else if(bytesCounted == bytesInFile)  {
            AddPeriod(lastBit);
            lastBit = NOT_SET;
            return BITS_IN_BYTE;
        }
        for (i; i < BITS_IN_BYTE; i++)
            if (byte[i] == byte[startIndex]){
                tempPeriod.period++;
                if (i == BITS_IN_BYTE - 1)
                    bytesCounted++;
            } else {
                AddPeriod(byte[startIndex]);
                return i;
            }
        lastBit = byte[i - 1];
        if(bytesCounted != bytesInFile)
            return i;
        else 
            return i - 1;            
    }
    bytesCounted = 0;
    while (bytesRead) {
        for (int i = 0; i < bytesRead; i++){
            int bit;
            int k = 0;
            for(int j = 7; j >= 0; j--){
                bit = (1 << j & buffer[i]) != 0;
                byte[k++] = bit;         
            }
            int j = 0;
            while (j < 8){
                j = CountPeriods(j);
            }
        }
        if (bytesRead == BUFFER_SIZE && bytesInFile != bytesReadTotal) { //only if there was enough bytes during the first reading do we refill the buffer
            bytesRead = read(FCurr, buffer, BUFFER_SIZE);
            bytesReadTotal += bytesRead;
        }
        else 
            bytesRead = 0;
        PeriodListSort(&ListArr[0], PeriodCompareSort);
        PeriodListSort(&ListArr[1], PeriodCompareSort);
    }
    char *outBuffer = (char *)malloc(OUTPUT_BUFFER);
    //double bytesCheck = 0;
    sprintf(outBuffer, "%d %s ", getpid(), filePath);
    for(int i = 0; i < 2; i++){
        sprintf(errLine, "%d: ",i);
        strcat(outBuffer, errLine);
        for (int j = 0; j < ListArr[i].itemCount; j++){        
            sprintf(errLine, "%d*%lld ", ListArr[i].pointer[j].period, ListArr[i].pointer[j].count);
            strcat(outBuffer, errLine);
            //bytesCheck += (double)(ListArr[i].pointer[j].period * ListArr[i].pointer[j].count) / BITS_IN_BYTE;
        }
    }
    sprintf(errLine, "\n");
    strcat(outBuffer, errLine);
    fputs(outBuffer, stdout);
    free(outBuffer);
    //printf("\nЧисло байт полученное подсчетом периодов = %.3f\nРазмер файла = %lld\n", bytesCheck, bytesInFile);
    free(ListArr[0].pointer);
    free(ListArr[1].pointer);
    free(buffer);
    close(FCurr);
    return 0;
}

int CreateProcess(int errorLog, char *filePath, char *programName, int *maxProc, int *runningProc){
    int status;
    pid_t pid;
    while (*runningProc >= *maxProc - 1) {
        wait(&status);
        *runningProc -= 1;
    }
    if ((FCurr = open(filePath, O_RDONLY)) == -1) {
        snprintf(errLine, 8192, "%d %s %s\n", getpid(), filePath, strerror(errno));
        BufferedWrite(errorLog, errorBuffer, errLine);
        return 1;
    }
    *runningProc += 1;
    pid = fork();
    if (pid == 0){
        free(errorBuffer);
        BitCount(errorLog, filePath, programName);
        exit(0);
    }
    close(FCurr);
}
