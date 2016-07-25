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
#include <signal.h>
#include <ctype.h>

#include "msg_struct.h"

char path_to_server[PATHLENGTH];
char path_to_client[PATHLENGTH];
char present_working_dir[PATHLENGTH];
char temp_present_working_dir[PATHLENGTH];
char path_to_new_dir[PATHLENGTH];
struct stat testserver;
struct stat testclient;
int serverfifo;
int clientfifo;
int dummy;
int userid;
MSGSVR empty_msg_to_server;
MSGCLI empty_msg_to_client;



void initialize_empty_msgsvr()
{
    empty_msg_to_server.client_pid=0;
    empty_msg_to_server.instruct_code=0;
    empty_msg_to_server.msg[0]='\0';
}

void initialize_empty_msg_to_cli()
{
    empty_msg_to_client.error_code=0;
    empty_msg_to_client.msg[0]='\0';
}

void cleanup(int signaltype)
{
    close(serverfifo);

    close(dummy);
    unlink(path_to_client);
    fprintf(stdout,"Client %d is exiting...\n",getpid());
    exit(0);
}

char * first_non_whitespace(char * str)
{
    while(isspace(*str))
        str++;
    return str;
}



void read_msg_for_client( int prev, int next)
{
    if(prev==0)
    clientfifo=open(path_to_client,O_RDONLY);
    if(clientfifo)
    {
        fprintf(stdout,"Waiting for message from server to client ID : %d...\n",getpid());
        
        initialize_empty_msg_to_cli();
        
        if(read(clientfifo,&empty_msg_to_client,sizeof(empty_msg_to_client)))
        {
            fprintf(stdout,"Error code received from Server %d\n",empty_msg_to_client.error_code);
            fprintf(stdout,"Message from Server to Client: %s",empty_msg_to_client.msg);
            if(empty_msg_to_client.error_code==0)
                strcpy(present_working_dir,temp_present_working_dir);

        }
            if(next==0)
            close(clientfifo);
    }
}

void set_echo_user()
{
    //SET the user id and echo the user id and root directory.
    
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        fprintf(stdout,"Enter the user id:");
        scanf("%d%*c",&userid);
        
        
        empty_msg_to_server.userid=userid;
        empty_msg_to_server.client_pid=getpid();
        empty_msg_to_server.instruct_code=1;
        strcpy(present_working_dir,"/");
        strcpy(temp_present_working_dir,present_working_dir);
        strcpy(empty_msg_to_server.msg,temp_present_working_dir);
        write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
        read_msg_for_client(0,0);

    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}

void present_wd_ls_cli()
{
    //Print the present working directory.
    
    int c;
    
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stdout,"***Enter the userid first.\n");
        else
        {
            empty_msg_to_server.userid=userid;
            empty_msg_to_server.client_pid=getpid();
            empty_msg_to_server.instruct_code=2;

            strcpy(temp_present_working_dir,present_working_dir);
            strcpy(empty_msg_to_server.msg,temp_present_working_dir);
            
            fprintf(stdout,"Want to see whole list?\n 1.Yes \n 2.No\n");
            fprintf(stdout,"Enter the choice:");
            scanf("%d%*c",&c);
            
            if(c==1)
                empty_msg_to_server.sub_instruction=1;
            else
                empty_msg_to_server.sub_instruction=0;

            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client(0,1);
            if(empty_msg_to_server.sub_instruction==1)
                read_msg_for_client(1,0);
        }
        
    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}

void change_dir_cli()
{
    //SET new directory
    
    
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stdout,"***Enter the userid first.\n");
        else
        {
            empty_msg_to_server.userid=userid;
            empty_msg_to_server.client_pid=getpid();
            empty_msg_to_server.instruct_code=3;
            
            printf("Enter the directory to which you like to change:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            //fprintf(stdout,"temp_present %s",temp_present_working_dir);
            
            if(temp_present_working_dir[0]=='/')
                ;
            else
            {
                strcpy(path_to_new_dir,temp_present_working_dir);
                
                strcpy(temp_present_working_dir,present_working_dir);
                strcat(temp_present_working_dir, path_to_new_dir);
            }
            strcpy(empty_msg_to_server.msg,temp_present_working_dir);
            
            
            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client(0,0);
        }
        
    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}



void menu()
{
    if(mkfifo(path_to_client,MKFIFO_FILE_MODE))
    {
        fprintf(stderr,"Error with client FIFO creation.\n");
    }
    else
    {
        fprintf(stdout,"Client FIFO is created with path %s\n",path_to_client);
        



        int c=99;
        while(c)
        {
            fprintf(stdout,"1.Enter the user id and echo\n");
            fprintf(stdout,"2.Present directory and current list.\n");
            fprintf(stdout,"3.Change directory.\n");
            fprintf(stdout,"0.Exit\n");
            fprintf(stdout,"Enter your choice:");
            scanf("%d%*c",&c);
            switch(c)
            {
                case 1:
                    set_echo_user();
                    break;
                case 2:
                    present_wd_ls_cli();
                    break;
                case 3:
                    change_dir_cli();
                    break;
                    
                case 0:
                    fprintf(stdout,"Existing from client.\n");
                    cleanup(SIGINT);
                
            }
        }
    }
    
}

int main(int argc, const char * argv[]) {
    // insert code here...
    
    signal(SIGINT,cleanup);
    
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
                    serverfifo=open(path_to_server,O_WRONLY);
                    if(serverfifo)
                    menu();
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
            
            cleanup(SIGINT);
            
        }
        else
        {
            fprintf(stderr,"Some problem with server fifo\n");
        }
        
        
    }
    
    return 0;
}
