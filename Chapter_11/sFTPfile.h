#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ASCII_read(char *filename, char *buf, int *bytes);

void ASCII_write(char *filename, char *buf);

#define MAXLEN 1024

void BIN_read(char *filename, unsigned char *buf, int *bytes);

void BIN_write(char *filename, unsigned char *buf);