//
//  main.c
//  SimpleClientServerFIFO
//
//  Created by Sankarsan Seal on 24/07/16.
//  Copyright © 2016 Sankarsan Seal. All rights reserved.
//

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../SimpleClient/SimpleClient/msg_struct.h"
char path_to_fifo[1024];
int server_fifo;
int dummy;
struct stat test;
MSGSVR empty_msg_to_server;

void initialize_empty_msgsvr()
{
    empty_msg_to_server.clientid=0;
    empty_msg_to_server.instruct_code=0;
    empty_msg_to_server.msg[0]='\0';
}


void cleanup(int signaltype)
{
    if(server_fifo)
    close(server_fifo);
    unlink(path_to_fifo);
    printf("\nExiting..\n");
    exit(0);
}

void readfromfifo()
{
    initialize_empty_msgsvr();

    while(read(server_fifo,&empty_msg_to_server,sizeof(empty_msg_to_server)))
    {
        fprintf(stdout,"Instruction received from client : %d",empty_msg_to_server.instruct_code);
        fprintf(stdout, "Client ID : %d", empty_msg_to_server.clientid);
        fprintf(stdout,"Message send %s",empty_msg_to_server.msg);
    }
}


void start_server()
{
    server_fifo=open(path_to_fifo,O_RDONLY);
    dummy=open(path_to_fifo,O_WRONLY);
    if(!server_fifo)
    {
        fprintf(stderr,"Error in opening server fifo: %s\n",strerror(errno));
        cleanup(SIGINT);
    }
    else
    {
        readfromfifo();
        fprintf(stdout,"Starting server...\n");
        
    }
}


int main(int argc, const char * argv[]) {
    // insert code here...
    
    signal(SIGINT,cleanup);
    
    if(argc!=2)
        printf("usage:%s <path to FIFO file>\n",argv[0]);
    else
    {
        strcpy(path_to_fifo,argv[1]);

        if(stat(path_to_fifo,&test)==0)
        {
            printf("Path exists.\n");
            switch(test.st_mode & S_IFMT)
            {
                case S_IFDIR:
                    printf("It is directory\n");
                    break;
                case S_IFIFO:
                    printf("It is FIFO\n");
                    start_server();
                    break;
                case S_IFREG:
                    printf("It is Regular\n");
                    printf("Please remove or rename the existing regular file\n");
                    break;
            }
        }
        else
        {
            fprintf(stderr,"Something went wrong: %s\n",strerror(errno));
            fprintf(stdout,"Creating new FIFO with given path\n");
            
            if(mkfifo(path_to_fifo,S_IRUSR|S_IRGRP|S_IWUSR|S_IROTH))
                fprintf(stderr,"mkfifo error :%s\n",strerror(errno));
            else
            start_server();
        }
    }
    
    
    return 0;
}
