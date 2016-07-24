//
//  main.c
//  SimpleClient
//
//  Created by Sankarsan Seal on 24/07/16.
//  Copyright © 2016 Sankarsan Seal. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "msg_struct.h"

char path_to_server[1024];
char path_to_client[1024];
struct stat testserver;
struct stat testclient;
int serverfifo;
int clientfifo;
MSGSVR empty_msg_to_server;



void send_msg()
{
    serverfifo=open(path_to_server,O_WRONLY);
    
    
    if(serverfifo)
    {
        empty_msg_to_server.client_pid=getpid();
        empty_msg_to_server.instruct_code=2;
        strcpy(empty_msg_to_server.msg,"Client Message sent successfully.\n");
        write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
        
    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}

int main(int argc, const char * argv[]) {
    // insert code here...
    if(argc!=2)
        fprintf(stderr,"usage:%s <path to server fifo>\n",argv[0]);
    else
    {
        strcpy(path_to_server,argv[1]);
        sprintf(path_to_client,"/tmp/client.%d",getpid());
        if(stat(path_to_server,&testserver)==0)
        {
            printf("Server Path exists\n");
            switch(testserver.st_mode & S_IFMT)
            {
                case S_IFIFO:
                    fprintf(stdout,"Server path is actually FIFO\n");
                    send_msg();
                    break;
                case S_IFREG:
                    fprintf(stderr,"Server path is pointing to a regular file.\n");
                    fprintf(stderr,"Please remove or rename the file.\n");
                    break;
                case S_IFDIR:
                    fprintf(stderr,"Server path is pointing to directory.\n");
                    break;
                default:
                    fprintf(stderr,"Unknown file type.\n");
                    
            }
        }
        else
        {
            fprintf(stderr,"Some problem with server fifo\n");
        }
        
        
    }
    
    return 0;
}
