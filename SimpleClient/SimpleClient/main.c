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
char new_renamed_file_dir[PATHLENGTH];
char path_to_new_dir[PATHLENGTH];
struct stat testserver;
struct stat testclient;
int serverfifo;
int clientfifo;
int dummy;
int userid;
int last_inode_used;
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
    empty_msg_to_client.last_inode_used=0;
    empty_msg_to_client.more=0;
}

void client_termination_msg()
{
    //SET the user id and echo the user id and root directory.
    
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        //fprintf(stdout,"Enter the user id:");
        //scanf("%d%*c",&userid);
        
        
        empty_msg_to_server.userid=userid;
        empty_msg_to_server.client_pid=getpid();
        empty_msg_to_server.instruct_code=99;
        empty_msg_to_server.last_inode_used=1;
//        strcpy(present_working_dir,"/");
        strcpy(empty_msg_to_server.msg,present_working_dir);
        write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
//        read_msg_for_client();
        
    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}


void cleanup(int signaltype)
{


    close(dummy);
    if(stat(path_to_server,&testserver)!=0)
    {
        unlink(path_to_client);
    }
    else
    {
        fprintf(stdout,"Sending termination request to server...\n");
        client_termination_msg();
    }
    close(serverfifo);
    fprintf(stdout,"Client %d is exiting...\n",getpid());
    exit(0);
}

char * first_non_whitespace(char * str)
{
    while(isspace(*str))
        str++;
    return str;
}



void read_msg_for_client()
{
    int more=1;

    clientfifo=open(path_to_client,O_RDONLY);
    if(clientfifo)
    {
        fprintf(stdout,"Waiting for message from server to client ID : %d...\n",getpid());
        while(more)
        {
        initialize_empty_msg_to_cli();
        
            if(read(clientfifo,&empty_msg_to_client,sizeof(empty_msg_to_client)))
            {
                fprintf(stdout,"Error code received from Server : %d\n",empty_msg_to_client.error_code);
                fprintf(stdout,"Message from Server to Client: %s\n",empty_msg_to_client.msg);
                if(empty_msg_to_client.error_code==0)
                {
                    last_inode_used=empty_msg_to_client.last_inode_used;
                    if(empty_msg_to_client.ischangedirinst==1)
                    {
                        if(strcmp(present_working_dir,"/"))
                        {
                        
                            if(!strcmp(empty_msg_to_client.msg,"."))
                                ;
                        
                            else if(!strcmp(empty_msg_to_client.msg,".."))
                        
                            {
                                
                            
                                *(strrchr(present_working_dir,'/'))='\0';
                                if(!strcmp(present_working_dir,""))
                                    strcpy(present_working_dir,"/");
                        
                            }
                            else
                            {
                                strcat(present_working_dir,"/");
                                strcat(present_working_dir,empty_msg_to_client.msg);
 
                            }
                        }
                        else
                        {
                            if(strcmp(empty_msg_to_client.msg,".")&&strcmp(empty_msg_to_client.msg,".."))
                                strcat(present_working_dir,empty_msg_to_client.msg);
                        }
 
                    }
                }
                more=empty_msg_to_client.more;

            }
        }

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
        empty_msg_to_server.last_inode_used=1;
        strcpy(present_working_dir,"/");
        strcpy(empty_msg_to_server.msg,present_working_dir);
        write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
        read_msg_for_client();

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
            empty_msg_to_server.last_inode_used=last_inode_used;


            strcpy(empty_msg_to_server.msg,present_working_dir);
            
            fprintf(stdout,"Want to see whole list?\n 1.Yes \n 2.No\n");
            fprintf(stdout,"Enter the choice:");
            scanf("%d%*c",&c);
            
            if(c==1)
                empty_msg_to_server.sub_instruction=1;
            else
                empty_msg_to_server.sub_instruction=0;

            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client();
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
            fprintf(stderr,"***Enter the userid first.\n");
        else
        {
            empty_msg_to_server.userid=userid;
            empty_msg_to_server.client_pid=getpid();
            empty_msg_to_server.instruct_code=3;
            empty_msg_to_server.last_inode_used=last_inode_used;
            empty_msg_to_server.sub_instruction=0;
            
            printf("Enter the directory to which you like to change:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            
            //fprintf(stdout,"temp_present %s",temp_present_working_dir);
            
  /*          if(temp_present_working_dir[0]=='/')
                ;
            else
            {
                strcpy(path_to_new_dir,temp_present_working_dir);
                
                strcpy(temp_present_working_dir,present_working_dir);
                strcat(temp_present_working_dir, path_to_new_dir);
            }
   */
            strcpy(empty_msg_to_server.msg,temp_present_working_dir);
            
            
            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client();
        }
        
    }
    else
        fprintf(stderr,"Message to server is not send.\n");
    
    
    
}

void make_dir_cli()
{
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stdout,"***Enter the userid first.\n");
        else
        {
            empty_msg_to_server.userid=userid;
            empty_msg_to_server.client_pid=getpid();
            empty_msg_to_server.instruct_code=4;
            empty_msg_to_server.last_inode_used=last_inode_used;
            empty_msg_to_server.sub_instruction=0;

            
            printf("Enter the name of directory which you like to create:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            
            //fprintf(stdout,"temp_present %s",temp_present_working_dir);
            
           
            strcpy(empty_msg_to_server.msg,temp_present_working_dir);


            
            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client();
        }

        
    }
    else
    {
        fprintf(stderr,"Message to server is not send.\n");

    }
}

void remove_dir_cli()
{
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stderr,"***Enter the userid first.\n");
        else
        {
         
            
            printf("Enter the name of directory which you like to delete:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            if(strcmp(temp_present_working_dir,".") && strcmp(temp_present_working_dir, ".."))
            {
            
            empty_msg_to_server.userid=userid;
            empty_msg_to_server.client_pid=getpid();
            empty_msg_to_server.instruct_code=5;
            empty_msg_to_server.last_inode_used=last_inode_used;
            empty_msg_to_server.sub_instruction=0;
            
            
            
            //fprintf(stdout,"temp_present %s",temp_present_working_dir);
            
            
            strcpy(empty_msg_to_server.msg,temp_present_working_dir);
            
            
            
            write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
            read_msg_for_client();
            }
            else
            {
                fprintf(stderr,"***Can not delete . or ..\n");
                
            }
        }
        
        
    }
    else
    {
        fprintf(stderr,"Message to server is not send.\n");
        
    }

}

void create_file_cli()
{
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stderr,"***Enter the userid first.\n");
        else
        {
            
            
            printf("Enter the name of file which you like to create:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            if(strcmp(temp_present_working_dir,".") && strcmp(temp_present_working_dir, ".."))
            {
                
                empty_msg_to_server.userid=userid;
                empty_msg_to_server.client_pid=getpid();
                empty_msg_to_server.instruct_code=6;
                empty_msg_to_server.last_inode_used=last_inode_used;
                empty_msg_to_server.sub_instruction=0;
                
                
                
                //fprintf(stdout,"temp_present %s",temp_present_working_dir);
                
                
                strcpy(empty_msg_to_server.msg,temp_present_working_dir);
                
                
                
                write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
                read_msg_for_client();
            }
            else
            {
                fprintf(stderr,"***Can not create file . or ..\n");
                
            }
        }
        
        
    }
    else
    {
        fprintf(stderr,"Message to server is not send.\n");
        
    }
    
}

void delete_file_cli()
{
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stderr,"***Enter the userid first.\n");
        else
        {
            
            
            printf("Enter the name of file which you like to delete:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            if(strcmp(temp_present_working_dir,".") && strcmp(temp_present_working_dir, ".."))
            {
                
                empty_msg_to_server.userid=userid;
                empty_msg_to_server.client_pid=getpid();
                empty_msg_to_server.instruct_code=7;
                empty_msg_to_server.last_inode_used=last_inode_used;
                empty_msg_to_server.sub_instruction=0;
                
                
                
                //fprintf(stdout,"temp_present %s",temp_present_working_dir);
                
                
                strcpy(empty_msg_to_server.msg,temp_present_working_dir);
                
                
                
                write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
                read_msg_for_client();
            }
            else
            {
                fprintf(stderr,"***Can not delete file . or ..\n");
                
            }
        }
        
        
    }
    else
    {
        fprintf(stderr,"Message to server is not send.\n");
        
    }
    
}

void rename_cli()
{
    if(serverfifo)
    {
        initialize_empty_msgsvr();
        if(userid==0)
            fprintf(stderr,"***Enter the userid first.\n");
        else
        {
            
            
            printf("Enter the name of file/directory which you like to rename:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            //sprintf(stdout,"%s\n",path_to_new_dir);
            //sprintf(temp_present_working_dir,")
            
            strcpy(temp_present_working_dir, first_non_whitespace(path_to_new_dir));
            
            printf("Enter the name to which you like to rename:\n");
            scanf("%[^\n]%*c",path_to_new_dir);
            
            strcpy(new_renamed_file_dir,first_non_whitespace(path_to_new_dir));
            
            if(strcmp(temp_present_working_dir,".") && strcmp(temp_present_working_dir, ".."))
            {
                
                empty_msg_to_server.userid=userid;
                empty_msg_to_server.client_pid=getpid();
                empty_msg_to_server.instruct_code=8;
                empty_msg_to_server.last_inode_used=last_inode_used;
                empty_msg_to_server.sub_instruction=0;
                
                
                
                //fprintf(stdout,"temp_present %s",temp_present_working_dir);
                
                
                strcpy(empty_msg_to_server.msg,temp_present_working_dir);
                strcat(empty_msg_to_server.msg,"#");
                strcat(empty_msg_to_server.msg,new_renamed_file_dir);
                
                
                
                write(serverfifo,&empty_msg_to_server,sizeof(empty_msg_to_server));
                read_msg_for_client();
            }
            else
            {
                fprintf(stderr,"***Can not delete file . or ..\n");
                
            }
        }
        
        
    }
    else
    {
        fprintf(stderr,"Message to server is not send.\n");
        
    }
    
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
            fprintf(stdout,"4.Make Directory.\n");
            fprintf(stdout,"5.Remove Directory.\n");
            fprintf(stdout,"6.Create File.\n");
            fprintf(stdout,"7.Delete File.\n");
            fprintf(stdout,"8.Rename File or Directory.\n");
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
                case 4:
                    make_dir_cli();
                    break;
                case 5:
                    remove_dir_cli();
                    break;
                case 6:
                    create_file_cli();
                    break;
                case 7:
                    delete_file_cli();
                    break;
                case 8:
                    rename_cli();
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
