/***** pipetest.c *****/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#define MAX_LINE 80
int main()
{
    int thePipe[2], ret;
    char buf[MAX_LINE + 1];
    const char *testbuf = "a test string.";
    if (pipe(thePipe) == 0)
    {
        if (fork() == 0)
        {
            ret = read(thePipe[0], buf, MAX_LINE);
            buf[ret] = '\0';
            printf("Child read %s\n", buf);
        }
        else
        {
            ret = write(thePipe[1], testbuf, strlen(testbuf));
            ret = wait(NULL);
        }
    }
    close(thePipe[0]);
    close(thePipe[1]);
    return 0;
}