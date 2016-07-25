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
char path_to_fifo[PATHLENGTH];
int server_fifo;
int dummy;
struct stat test;
pthread_t tid;
MSGSVR * empty_msg_to_server;
pthread_rwlock_t rwlock;


int share_var;

MSGSVR * initialize_empty_msgsvr()
{
    MSGSVR * temp=NULL;
    
    if((temp=(MSGSVR *)malloc(sizeof(MSGSVR))))
    {
        temp->client_pid=0;
        temp->instruct_code=0;
        temp->msg[0]='\0';
    }
    
    return temp;
}

MSGCLI * initialize_empty_msg_to_cli()
{
    MSGCLI * temp=NULL;
    if((temp=(MSGCLI *)malloc(sizeof(MSGCLI))))
    {
    temp->error_code=0;
    temp->msg[0]='\0';
    }
    
    return temp;
}



void cleanup(int signaltype)
{
    if(server_fifo)
    close(server_fifo);
    pthread_rwlock_destroy(&rwlock);
    unlink(path_to_fifo);
    printf("\nExiting..\n");
    exit(0);
}

void * echo_userid(void * argv)
{
    MSGSVR * temp =(MSGSVR *) argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    int cli_fifo;
    
    if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
    {
        sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
    
        cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
    
            {
        
                if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                {
                    empty_msg_from_svr_to_cli->error_code=0;
                    sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *present working directory:\n %s\n",temp->userid,temp->client_pid,temp->msg);
                    write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                    free(empty_msg_from_svr_to_cli);
                }

            close(cli_fifo);
            }
    if(temp)
        free(temp);

    }
    return NULL;
    
}

void * present_wd_ls(void * argv)
{
    MSGSVR * temp =(MSGSVR *)argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    int cli_fifo;

    
    if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
    {
        sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
        
        cli_fifo=open(path_to_cli_fifo,O_WRONLY);
        if(cli_fifo)
            
        {
            
            if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
            {
                empty_msg_from_svr_to_cli->error_code=0;
                sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *present working directory:\n %s\n",temp->userid,temp->client_pid,temp->msg);
               
                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                free(empty_msg_from_svr_to_cli);
                
                if(temp->sub_instruction==1)
                {
                
                if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                empty_msg_from_svr_to_cli->error_code=1;
                strcpy(empty_msg_from_svr_to_cli->msg,"No directory listing available.\n");
                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                free(empty_msg_from_svr_to_cli);
                }
                
            }
            close(cli_fifo);
        }
        if(temp)
            free(temp);

    }
    return NULL;
    
}

void * change_dir(void * argv)
{
    MSGSVR * temp =(MSGSVR *)argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    int cli_fifo;
    
    
    if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
    {
        sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
        
        cli_fifo=open(path_to_cli_fifo,O_WRONLY);
        if(cli_fifo)
            
        {
            if(!pthread_rwlock_wrlock(&rwlock))
            {
            
                if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                {
                    fprintf(stdout,"Waiting to update shared variable:");
                    scanf("%d",&share_var);
                empty_msg_from_svr_to_cli->error_code=0;
                sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *present working directory:\n %s\n shared var value %d\n",temp->userid,temp->client_pid,temp->msg,share_var);
                
                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                free(empty_msg_from_svr_to_cli);
                
                
                }
            pthread_rwlock_unlock(&rwlock);

            }
            else
                fprintf(stdout,"Can not get write lock.\n");
        close(cli_fifo);
        }
        if(temp)
            free(temp);
        
    }
    return NULL;
    
}



void readfromfifo()
{
    empty_msg_to_server=initialize_empty_msgsvr();
    
    if(empty_msg_to_server)
    {
    
        while(read(server_fifo,empty_msg_to_server,sizeof(MSGSVR)))
        {
            
        fprintf(stdout,"Instruction Code received from client : %d\n",empty_msg_to_server->instruct_code);
        fprintf(stdout, "Client PID : %d\n", empty_msg_to_server->client_pid);
        fprintf(stdout,"Message sent %s\n",empty_msg_to_server->msg);
            switch(empty_msg_to_server->instruct_code)
            {
            
                case 1: //echo userid and process id
                    pthread_create(&tid,NULL,echo_userid, empty_msg_to_server);
                    break;
                case 2: //Listing present working directory
                    pthread_create(&tid, NULL, present_wd_ls, empty_msg_to_server);
                    break;
                case 3:
                    pthread_create(&tid,NULL,change_dir,empty_msg_to_server);
                    break;
                
                    
                    
                    
                
            }
            
            if((empty_msg_to_server=initialize_empty_msgsvr()))
                continue;
            else
            {
                fprintf(stderr,"Problem with allocation memory\n");
                cleanup(SIGINT);
                break;
            }
            
            
        }
    }
    else
    {
        fprintf(stderr,"Problem with allocating new space for message.\n");
    }
}

//After successfully checking available  server FIFO  Server is started

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
        fprintf(stdout,"Starting server...\n");
        pthread_rwlock_init(&rwlock,NULL);

        readfromfifo();
        
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
            printf("Given server path already exists.\n");
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
            
            if(mkfifo(path_to_fifo,MKFIFO_FILE_MODE))
                fprintf(stderr,"mkfifo error :%s\n",strerror(errno));
            else
            start_server();
        }
    }
    
    
    return 0;
}
