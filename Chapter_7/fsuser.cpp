#include "fsheader.h"
#include <iostream>
#include <string.h>
#include <fstream>
using namespace std;

// g++ -o fsuser fsuser.cpp fsoperation.cpp fsheader.h

extern char curname[14];
extern int road[20];
extern int num;
extern char auser[6];
fstream control;
int main(){
    control.open("control.txt", ios::in | ios::out);
    int i;
    control >> i;
    control.close();
    if(i!=0){
        initial();
    }
    control.open("control.txt", ios::in|ios::out);
    if(!control){
        cout << "can't open control.txt" << endl;
        exit(2);
    }
    control.seekp(0);
    control << 0;
    control.close();
    strcpy(curname, "root");
    road[0]=0;
    num = 1;
    cout << "请登录系统" << endl;
    while(!login()) cout << "wrong!" << endl;
    cout << "login success" << endl;
    cout << "*******Welcom " << auser << "*******" << endl;
    readsuper();
    getcommand();
    writesuper();
    return 0;
}