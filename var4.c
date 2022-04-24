#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

#define BYTES_MAX_COUNT 255

int ParseN(const char* str);
int IsHex(char *str);
char* FormFullPath(char* dirName, char* fileName);
int IsRegularFile(char* dirName, char* fileName);
void TransformHexStringToByteArray(char* hexStr);
int IsSequenceEqual(char* dest, char* src, int len);
long long FindByteSequence(char* byteArr, int arrLen, char* fileFullPath);

int main(int argc,char *argv[])
{
    if (argc != 4) 
    {
        perror("There must be 3 params:\n");
        perror("1 - dir name\n");
        perror("2 - byte sequance\n");
        perror("3 - number of working at the same time processes\n");
        return -1;
    }

    char * dirName = argv[1];
    DIR * dir = opendir(dirName);
    if (dir == NULL)
    {
        fprintf(stderr,"Can't open directory %s\n",dirName);
        return -1;
    }

    char *bytesSequance = argv[2];
    if (!IsHex(bytesSequance))
        return -1;
    
    int N = ParseN(argv[3]);
    if (N <=0)
        return -1;
    
    struct dirent* fileInfo = NULL;
    int procCount = 0;
    
    int arrLen = strlen(bytesSequance);
    arrLen = arrLen / 2 + arrLen % 2;
    TransformHexStringToByteArray(bytesSequance);
    
    do
    {
        while (procCount < N && (fileInfo = readdir(dir)) != NULL)
        {
            if (IsRegularFile(dirName, fileInfo->d_name)==0)
            {
                
                pid_t pid = fork();
                if (pid > 0)
                    procCount++;
                else if (pid == 0)
                {                   
                    char* fullPath = FormFullPath(dirName, fileInfo->d_name);
                    long long bytesEqual = FindByteSequence(bytesSequance, arrLen, fullPath);
                    pid_t pid = getpid();
                    
                    if (bytesEqual != -1)
                        printf("File name: %s\nPID: %d\nFound equals sequances: %lld\n\n", fullPath, pid, bytesEqual);                  
                    free(fullPath);

                    exit(EXIT_SUCCESS);
                }
                else
                    perror("Can't start the process");
                
            }
        }

        int status;
        if (wait(&status) != -1 )
        {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                procCount--;
        }
    }while (fileInfo != NULL);

    while (waitpid(-1, NULL, WNOHANG) != -1);
    
    if (closedir(dir))
    {
        fprintf(stderr,"Can't close directory %s",dirName);
        return -1;
    }
    return 0;   
}

int ParseN(const char* str)
{
    char *endptr;
	errno = 0;
	int res = strtol(str,&endptr,10);
	if ((errno == ERANGE && (res == LONG_MAX || res == LONG_MIN))
                   || (errno != 0 && res == 0))
        {
            perror("strtol");
            exit(EXIT_FAILURE);
        }
			
	if (endptr == str) 
    {
        fprintf(stderr, "No digits were found in N\n");
        exit(EXIT_FAILURE);
    }    	

	if (res<=0)
    {
        perror("number of working at the same time processes must be > 0\n");
        exit(EXIT_FAILURE);
    }
    
    return res;
}

int IsHex(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
        if (!(str[i] >= '0' && str[i] <= '9') && !(str[i] >= 'A' && str[i] <= 'F') && !(str[i] >= 'a' && str[i] <= 'f'))
        {
            perror("Error: Incorrect symbols in the bytes sequance\n Bytes should be represented in hex-format\n");
            return -1;
        }

    if (len > 2 * BYTES_MAX_COUNT){
        perror("Error: Bytes count should be less or equal then 255");
        len = 0;
    }
    return len;   
}

char* FormFullPath(char* dirName, char* fileName)
{
    char *fullPath= malloc(strlen(dirName) + strlen(fileName) + 2);
    strcpy(fullPath, dirName);
    
    if (fullPath[strlen(fullPath) - 1] != '/') 
        strcat(fullPath, "/");

    strcat(fullPath, fileName);
    return fullPath;
}

int IsRegularFile(char* dirName, char* fileName){
    char* fullPath = FormFullPath(dirName, fileName); 
    int res;
    struct stat fileInfo;
    if (stat(fullPath, &fileInfo))
    {
        fprintf(stderr,"Can't read statictics of %s",fullPath);
        res = -1;
    }
    else
        res = ( (fileInfo.st_mode & S_IFMT) == S_IFREG) ? 0 : -1; 
    
    free(fullPath);
    return res;
}

void TransformHexStringToByteArray(char* hexStr)
{
    
    int arrLen = strlen(hexStr);
    arrLen = arrLen / 2 + arrLen % 2;
   
    char byte[3] = {'\0', '\0', '\0'};
    for (int i = 0; i < arrLen; i++)
    {
        byte[0] = hexStr[2 * i];
        byte[1] = hexStr[2 * i + 1];

        hexStr[i] = strtol(byte, NULL, 16);
    }
}

int IsSequenceEqual(char* dest, char* src, int len)
{
    char res;
    int i = 0;
    
    do
        res = dest[i] - src[i];
    while((res == 0) && (++i < len));
    
    return res;
}

long long FindByteSequence(char* byteArr, int arrLen, char* fileFullPath)
{
    FILE* file = fopen(fileFullPath, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Can't open file %s\n", fileFullPath);
        return -1;
    }
    char buffer[BYTES_MAX_COUNT];

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);

    long long res = 0;

    for (long i = 0 ; i < fileSize; i++)
    {
        fseek(file, i, SEEK_SET);
        fread(buffer, arrLen, sizeof(char), file);
        if(IsSequenceEqual(buffer, byteArr, arrLen) == 0)
            res++;
    }

    if (fclose(file))
    {
        fprintf(stderr,"Can't close file %s\n",fileFullPath);
        return -1;
    }

    return res;
}

