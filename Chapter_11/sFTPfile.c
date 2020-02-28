#include "sFTPfile.h"

void ASCII_read(char *filename, char *buf, int *bytes){
    FILE *fp;
    int rc = 0;
    if((fp = fopen(filename, "r"))==NULL){
        printf("Not open.\n");
        exit(0);
    }
    char *str;
    while(str=fgets(buf+rc,1024-rc,fp)){
        rc += strlen(str);
    }
    fclose(fp);
    *bytes = rc;
}

void ASCII_write(char *filename, char *buf){
    FILE *fp;
    if((fp = fopen(filename, "w"))==NULL){
        printf("Cannot open file.\n");
        exit(0);
    }
    printf("filedata: \n%s\n", buf);
    fputs(buf, fp);
}

void BIN_read(char *filename, unsigned char *buf, int *bytes){
    FILE *fp;
    int rc;
    if((fp = fopen(filename, "rb"))==NULL){
        printf("Cannot open file.\n");
        exit(0);
    }
    while((rc=fread(buf,sizeof(unsigned char), MAXLEN, fp))!=0){

    }
    *bytes = rc;
    fclose(fp);
}

void BIN_write(char *filename, unsigned char *buf){
    FILE *fp;
    int rc;
    if((fp = fopen(filename, "wb"))==NULL){
        printf("Cannot open file.\n");
        exit(0);
    }
    fwrite(buf,sizeof(unsigned char), rc, fp);
}

// char buf[1024];

// int main(){
//     ASCII_read("./chong.txt", buf);
//     printf("%s",buf);
// }