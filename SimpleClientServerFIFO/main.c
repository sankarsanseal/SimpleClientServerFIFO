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
#include "inode_data_struct.h"
char path_to_fifo[PATHLENGTH];
char path_to_fsfile[PATHLENGTH];
int server_fifo;
int dummy;
struct stat test;
pthread_t tid;
MSGSVR * empty_msg_to_server;
pthread_rwlock_t inode_list_lock;
pthread_rwlock_t data_block_status_list_lock;
pthread_rwlock_t sb_lock;
size_t size_in_MB;
size_t size_in_bytes;
size_t blocksize;
int no_of_inodes;
int no_of_datablocks;
int no_of_directory_entry;
char c;
FILE * fsfile;
SB sb;
INODE * inode_list;
INODE empty_inode;
DBS * data_block_status_list;




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
        temp->more=0;
        temp->ischangedirinst=0;
        temp->last_inode_used=0;
    }
    
    return temp;
}

void write_back_filesystem()
{
    int i;
    fprintf(stdout,"\nFilesystem file closing is started.\n");
    
    fsfile=fopen(path_to_fsfile,"rb+");
    if(fsfile)
    {
        fprintf(stdout,"Next Inode : %d and next data block %d\n",sb.remembered_inode,sb.remembered_data_block);
        fseek(fsfile,0,SEEK_SET);
        fwrite(&sb,sizeof(SB),1,fsfile);
        for(i=0;i<sb.no_of_inode;i++)
            fwrite((inode_list+i),sizeof(INODE),1,fsfile);
        for(i=0;i<sb.no_of_data_blocks;i++)
            fwrite((data_block_status_list+i),sizeof(DBS),1,fsfile);
        
        fprintf(stdout,"Filesystem file is written successfully.\n");
        
        fclose(fsfile);
    }
    else
    {
        fprintf(stderr,"Can not open the filesystem file to write last time.\n");
    }
    
    if(fsfile)
        fclose(fsfile);
}

void cleanup(int signaltype)
{
    if(server_fifo)
    close(server_fifo);
    unlink(path_to_fifo);
    write_back_filesystem();
    
    pthread_rwlock_destroy(&inode_list_lock);
    pthread_rwlock_destroy(&data_block_status_list_lock);
    pthread_rwlock_destroy(&sb_lock);

    printf("\nExiting..\n");
    exit(0);
}

DIRS * initialize_dir_entry()
{
    DIRS * temp=NULL;
    temp=(DIRS *)malloc(sizeof(DIRS));
    if(temp)
    {
        temp->inode=0;
        temp->name[0]='\0';
    }
    
    return temp;
    
}

DIRENTRY * initialize_dir_pos()
{
    DIRENTRY * temp=NULL;
    
    temp=(DIRENTRY *)malloc(sizeof(DIRENTRY));
    if(temp)
    {
        temp->i=-1;
        temp->j=-1;
        temp->inode_ind=0;
    }
    
    
    
    return temp;
}

DIRENTRY * search_name(FILE * file, int inode_ind, char * str)
{
    int i,j,flag=0;

    DIRS * temp;
    DIRENTRY * pos=NULL;
    //pthread_rwlock_rdlock(&inode_list_lock);
    

    if(inode_list[inode_ind].lock==0)
    {
        for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
        {
            if(inode_list[inode_ind].data_block_offset[i]!=-1)
            {
                fseek(file,data_block_status_list[ inode_list[inode_ind].data_block_offset[i]].data_block_offset,SEEK_SET);
                for(j=0;j<sb.no_of_inode;j++)
                {
                    if((temp=initialize_dir_entry()))
                    {
                        fread(temp,sizeof(DIRS),1,file);
                        if(!strcmp(temp->name,str))
                        {
                            if((pos=initialize_dir_pos()))
                               {
                                   pos->i=i;
                                   pos->j=j;
                                   pos->inode_ind=temp->inode;
                                   flag=1;
                               }
                            break;
                        }
                        free(temp);
                    }
                    
                }
                
            }
            
            if(flag)
                break;

    
        }
    }
    else
    {
        fprintf(stderr,"Searched inode %d is locked.\n",inode_ind);
    }
    
    return pos;
}

/*int iname(char * path,int inode_ind, FILE * file)
{
    char * name;
    DIRENTRY * inode;
        if(path[0]=='/')
            inode=1;
        else
            inode=inode_ind;
    for(name=strtok(path,"/");name;name=strtok(NULL,"/"))
    {
        inode=search_name(file,inode,name);
        if(inode==0)
            break;
    }
    
    
    return inode;
}*/

int next_free_inode()
{
    int temp=0;
    int i;
   // pthread_rwlock_wrlock(&sb_lock);
   // pthread_rwlock_wrlock(&inode_list_lock);
    if(sb.remembered_inode!=0)
    temp=sb.remembered_inode;
    for(i=sb.remembered_inode+1;i<sb.no_of_inode;i++)
    {
            if((inode_list[i].type==0)&&(inode_list[i].lock==0) )
            {
                inode_list[i].lock=1;
                sb.remembered_inode=i;
                inode_list[i].lock=0;
                break;
            }
            else
                continue;
    }
   // pthread_rwlock_unlock(&inode_list_lock);
   // pthread_rwlock_unlock(&sb_lock);
    
    return temp;
    
}

int next_free_data_block()
{
    int temp=-1;
    int i;
   // pthread_rwlock_wrlock(&sb_lock);
   // pthread_rwlock_wrlock(&data_block_status_list_lock);
    
    if(sb.remembered_data_block!=-1)
    {
        temp=sb.remembered_data_block;
        data_block_status_list[sb.remembered_data_block].empty=0;
    }
    for(i=sb.remembered_data_block+1;i<sb.no_of_data_blocks;i++)
    {
        if((data_block_status_list[i].empty==1)&&(data_block_status_list[i].lock==0))
        {
            data_block_status_list[i].lock=1;
            sb.remembered_data_block=i;
            data_block_status_list[i].lock=0;
            break;
        }
        else
            continue;
    }
    if(i==sb.no_of_data_blocks)
        sb.remembered_data_block=-1;
   // pthread_rwlock_unlock(&data_block_status_list_lock);
   // pthread_rwlock_unlock(&sb_lock);
    
    return temp;
}


void dir_struct_creation(FILE * file,int inode_ind,int parent_inode_ind, int isroot)
{
    int i,j;
    DIRS * temp;
   // pthread_rwlock_rdlock(&inode_list_lock);

    if(inode_list[inode_ind].lock==1)
        fprintf(stderr,"Inode is locked. Please try after some time.\n");
    else
    {
        for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
        {
            if(inode_list[inode_ind].data_block_offset[i]!=-1)
            {
                fseek(file,data_block_status_list[inode_list[inode_ind].data_block_offset[i]].data_block_offset,SEEK_SET);
                //fprintf(stdout,"inode %d Ftell root : %lld\n",inode_list[inode_ind].data_block_offset[i],ftello(file));
                for(j=0;j<sb.no_of_directory_entry;j++)
                {
                    if((temp=initialize_dir_entry()))
                    {
                    
                        if((i==0)&&(j<=1))
                       {

                               if(j==0)
                               {
                                   strcpy(temp->name,".");

                                   if(isroot)
                                   {
                                       temp->inode=1;
                                   }
                                       
                                   else
                                   {
                                       temp->inode=inode_ind;
                                   }
                                }
                                if(j==1)
                                {
                                    strcpy(temp->name,"..");

                                    if(isroot)
                                    {
                                        temp->inode=1;
                                    }
                                    else
                                    {
                                    temp->inode=parent_inode_ind;
                                    }
                                }
                           inode_list[inode_ind].reference_count=2;
                           
                       }
                        fwrite(temp,sizeof(DIRS),1,file);
                        free(temp);

                    }
                    else
                    {
                        
                        fprintf(stderr,"Some problem with memory allocation while dir structure is created.\n");
                    }
                           
                    
                }
                       
            }
                
        }
    }
   // pthread_rwlock_unlock(&inode_list_lock);
}

void dir_data_block_allocation(int inode_ind,int type)
{
    int i;
   // pthread_rwlock_wrlock(&inode_list_lock);
    inode_list[inode_ind].lock=1;
    inode_list[inode_ind].type=type;

    inode_list[inode_ind].created=time(NULL);
    inode_list[inode_ind].modified=time(NULL);
    inode_list[inode_ind].accessed=time(NULL);
    
    for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
    {
        if((inode_list[inode_ind].data_block_offset[i]=next_free_data_block())!=-1)
        {
            
        }
        else
        {
            fprintf(stderr,"No Data block is presently available.\n");
        }
    }
    inode_list[inode_ind].lock=0;
    //pthread_rwlock_unlock(&inode_list_lock);
}

void dir_data_block_release(int inode_ind)
{
    int i;
    int data_block_ind;
    for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
    {
    
        if(inode_list[inode_ind].data_block_offset[i]!=0)
        {
            data_block_ind=inode_list[inode_ind].data_block_offset[i];
        //fprintf(stdout,"Data_Block ind : %d and sb.remembered data %d :\n",data_block_ind,sb.remembered_data_block);
            data_block_status_list[data_block_ind].lock=1;
            data_block_status_list[data_block_ind].empty=1;
            data_block_status_list[data_block_ind].lock=0;
        if(data_block_ind < sb.remembered_data_block)
            sb.remembered_data_block=data_block_ind;
        }
    }
    
}

DIRENTRY * find_next_free_dir_entry_slot(FILE * file, int inode_ind)
{
    int i,j,flag=0;
    DIRS * temp;
    DIRENTRY * pos=NULL;
    //pthread_rwlock_wrlock(&inode_list_lock);
    
    //inode_list[inode_ind].lock=1;

    for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
    {
        fseek(file,data_block_status_list[inode_list[inode_ind].data_block_offset[i]].data_block_offset,SEEK_SET);
        for(j=0;j<sb.no_of_directory_entry;j++)
        {
            if((temp=initialize_dir_entry()))
            {
                fread(temp,sizeof(DIRS),1,file);
                fprintf(stdout,"temp  node :%d temp name %s\n", temp->inode,temp->name);
                if(temp->inode==0)
                {
                    if((pos=initialize_dir_pos()))
                    {
                        pos->i=i;
                        pos->j=j;
                        flag=1;
                        
                    }
                    else
                    {
                        fprintf(stderr,"Error on dir pos allocation");
                        
                    }
                    break;
                }
                
                free(temp);
            }
        }
        if(flag==1)
            break;

    }
    //pthread_rwlock_unlock(&inode_list_lock);
    
    return pos;
    
}

void * client_ternimation(void * argv)
{
    MSGSVR * temp =(MSGSVR *) argv;
//    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
//    int cli_fifo;
    struct stat exist;
    
    if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
    {
        sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
        if(stat(path_to_cli_fifo,&exist)==0)
        {
            unlink(path_to_cli_fifo);
        }
        
    }
    if(temp)
        free(temp);
    
    return NULL;
    
}


void * echo_userid(void * argv)
{
    MSGSVR * temp =(MSGSVR *) argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    int cli_fifo;
    struct stat exist;
    
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
                    empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                    empty_msg_from_svr_to_cli->more=0;
                    empty_msg_from_svr_to_cli->ischangedirinst=0;
                    
                    if(stat(path_to_cli_fifo,&exist)==0)
                    write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                    free(empty_msg_from_svr_to_cli);
                }

            close(cli_fifo);
            }
    }
    if(temp)
        free(temp);
    
    return NULL;
    
}

void * present_wd_ls(void * argv)
{
    MSGSVR * temp =(MSGSVR *)argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    FILE * file;
    int cli_fifo;
    int inode_ind;
    int i,j;
    DIRS * tempdir;
    struct stat exist;
    
    pthread_rwlock_rdlock(&sb_lock);
    pthread_rwlock_rdlock(&inode_list_lock);
    pthread_rwlock_rdlock(&data_block_status_list_lock);
    
    file=fopen(path_to_fsfile,"rb");

    if(file)
    {
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
                
                    empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                    
                    empty_msg_from_svr_to_cli->ischangedirinst=0;
                    
                
                if(temp->sub_instruction==1)
                    empty_msg_from_svr_to_cli->more=1;
                else
                    empty_msg_from_svr_to_cli->more=0;
                    
                    if(stat(path_to_cli_fifo,&exist)==0)
                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                free(empty_msg_from_svr_to_cli);
                
                
                }
                if(temp->sub_instruction==1)
                {
                    inode_ind=temp->last_inode_used;
                    
                    //Read lock started here for inode list
                    //pthread_rwlock_rdlock(&inode_list_lock);
                    
                    for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
                    {
                        if(inode_list[inode_ind].data_block_offset[i]!=-1)
                        {
                            fseek(file,data_block_status_list[inode_list[inode_ind].data_block_offset[i]].data_block_offset, SEEK_SET);
                            
                            for(j=0;j<sb.no_of_directory_entry;j++)
                            {
                                if((tempdir=initialize_dir_entry()))
                                {
                                    fread(tempdir,sizeof(DIRS),1,file);
                                    if(tempdir->inode!=0)
                                    {
                                        
                                        if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                                            
                                        {
                                            sprintf(empty_msg_from_svr_to_cli->msg, "Inode no: %d | Name: %s\n",tempdir->inode,tempdir->name);
                                                
                                                empty_msg_from_svr_to_cli->error_code=0;
                                                empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                                empty_msg_from_svr_to_cli->more=1;
                                                    empty_msg_from_svr_to_cli->ischangedirinst=0;
                                            
                                            if(stat(path_to_cli_fifo,&exist)==0)
                                                write(cli_fifo, empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                                free(empty_msg_from_svr_to_cli);
                                                }
                                            
                                            }
                                            free(tempdir);
                                        }
                                        else
                                        {
                                        fprintf(stderr,"Problem with allocating space for dir structure.\n");
                                        }
                                        
                                    
                                    }
                                
                                }
                            }
                        
                            if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                            {
                                empty_msg_from_svr_to_cli->error_code=0;
                                empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                empty_msg_from_svr_to_cli->more=0;
                                empty_msg_from_svr_to_cli->ischangedirinst=0;
                                sprintf(empty_msg_from_svr_to_cli->msg,"End of directory listing\n");
                                
                                if(stat(path_to_cli_fifo,&exist)==0)
                                write(cli_fifo, empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                            free(empty_msg_from_svr_to_cli);
                            }
                    //pthread_rwlock_unlock(&inode_list_lock);
                    
                    
                }

            
            
                close(cli_fifo);
            
            }
        }
    }
    else
    {
        fprintf(stderr,"Error on opening filesystem file.\n");
    }
    if(temp)
        free(temp);
    if(file)
        fclose(file);
    

    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);
    return NULL;
    
}

void * change_dir(void * argv)
{
    MSGSVR * temp =(MSGSVR *)argv;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    int cli_fifo;
    DIRENTRY * pos;
    FILE * file;
    struct stat exist;
    
    pthread_rwlock_rdlock(&sb_lock);
    pthread_rwlock_rdlock(&inode_list_lock);
    pthread_rwlock_rdlock(&data_block_status_list_lock);


    file=fopen(path_to_fsfile,"rb");
    
    if(file)
    {
    
    
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
    
        {
        
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
        
        
        
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
        
            if(cli_fifo)
            
        
            {
            
                if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
            
                {
                    fprintf(stdout,"Temp msg at change dir %s\n",temp->msg);
                    
                    pos=search_name(file, temp->last_inode_used, temp->msg);
                    
                    if(pos)
                    {
                      if (inode_list[pos->inode_ind].type==1)
                      {
                          empty_msg_from_svr_to_cli->error_code=0;
                          empty_msg_from_svr_to_cli->last_inode_used=pos->inode_ind;
                          empty_msg_from_svr_to_cli->more=0;
                          empty_msg_from_svr_to_cli->ischangedirinst=1;
                          sprintf(empty_msg_from_svr_to_cli->msg,"%s",temp->msg);
                      }
                      else
                      {
                          empty_msg_from_svr_to_cli->error_code=1;
                          empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                          empty_msg_from_svr_to_cli->more=0;
                          empty_msg_from_svr_to_cli->ischangedirinst=0;
                          sprintf(empty_msg_from_svr_to_cli->msg,"%s is not directory.\n",temp->msg);
                      }
                        
                        
                        free(pos);
                        //pthread_rwlock_unlock(&data_block_status_list_lock);
                        //pthread_rwlock_unlock(&inode_list_lock);
                    }
                    else
                    {
                        empty_msg_from_svr_to_cli->error_code=1;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->more=0;
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        sprintf(empty_msg_from_svr_to_cli->msg,"%s directory is not found.\n",temp->msg);
                    }
                    

                
                    if(stat(path_to_cli_fifo,&exist)==0)
                    write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                
                    free(empty_msg_from_svr_to_cli);
                    }
            
                }
                close(cli_fifo);
            }
            if(temp)
                free(temp);
        }
    
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);

    return NULL;
    
}

void * make_dir(void * args)
{
    MSGSVR * temp=(MSGSVR *) args;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    char empty_str[32];
    struct stat exist;

    int cli_fifo;
    FILE * file;
    
    DIRENTRY * pos;

    DIRS * empty_entry;
    
    empty_str[0]='\0';
    
    pthread_rwlock_wrlock(&sb_lock);
    pthread_rwlock_wrlock(&inode_list_lock);
    pthread_rwlock_wrlock(&data_block_status_list_lock);

    
    file=fopen(path_to_fsfile,"rb+");
    
    if(file)
    {
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
        {
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
            

        
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
            
            {

                pos=search_name(file, temp->last_inode_used, temp->msg);

                    if(pos&&pos->inode_ind)
                    {
                        fprintf(stdout,"Inode ind %d\n",pos->inode_ind);
                        if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                        {

                            empty_msg_from_svr_to_cli->error_code=1;
                            sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *Can not create directory :\n %s\n Directory name already exists\n",temp->userid,temp->client_pid,temp->msg);
                            empty_msg_from_svr_to_cli->more=0;
                            empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                            empty_msg_from_svr_to_cli->ischangedirinst=0;
                            
                            if(stat(path_to_cli_fifo,&exist)==0)
                            write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                            free(empty_msg_from_svr_to_cli);
                    
                    
                        }
                        free(pos);
                    
                    }
                    else
                    {

                        fprintf(stdout,"last_inode_used:%d\n",temp->last_inode_used);
                        
                        if((pos=search_name(file, temp->last_inode_used, empty_str)))
                        {
                            fprintf(stdout,"pos i:%d j:%d\n",pos->i,pos->j);
                            fseek(file,data_block_status_list[ inode_list[temp->last_inode_used].data_block_offset[pos->i]].data_block_offset+sizeof(DIRS)*pos->j,SEEK_SET);
                            if((empty_entry=initialize_dir_entry()))
                            {
                                if((empty_entry->inode=next_free_inode()))
                                {
                                    fprintf(stdout,"empty_entry inode: %d\n",empty_entry->inode);
                                    
                                    strcpy(empty_entry->name,temp->msg);
                                    fwrite(empty_entry,sizeof(DIRS),1,file);

                                    dir_data_block_allocation(empty_entry->inode,1);
                                    dir_struct_creation(file, empty_entry->inode,temp->last_inode_used, 0);
                                    if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                                    {
                                        empty_msg_from_svr_to_cli->error_code=0;
                                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                        empty_msg_from_svr_to_cli->more=0;
                                        sprintf(empty_msg_from_svr_to_cli->msg,"New directory %s is created.\n",empty_entry->name);
                                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                                        
                                        if(stat(path_to_cli_fifo,&exist)==0)
                                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));

                                        free(empty_msg_from_svr_to_cli);
                                    }
                                    else
                                    {
                                        fprintf(stderr,"Error on allocating empty msg to client.\n");
                                    }
                                    
                                    inode_list[temp->last_inode_used].reference_count++;
                                }
                                else
                                {
                                    fprintf(stderr,"Error on new inode allocation.\n");
                                }
                                free(empty_entry);
                            }
                            else
                            {
                                fprintf(stderr,"Error in allocating directry entry.\n");
                            }
                            free(pos);
                            //pthread_rwlock_unlock(&inode_list_lock);
                        }
                        else
                        {
                            fprintf(stderr,"Error: Can not find any slot in present directory vacant");
                        }
                    
                    }
                //pthread_rwlock_unlock(&inode_list_lock);

                close(cli_fifo);
            }
        if(temp)
            free(temp);
        
        }
    }
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);

    return NULL;

    
}

void * remove_dir(void * args)
{
    MSGSVR * temp=(MSGSVR *) args;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    struct stat exist;
    
    int cli_fifo;
    FILE * file;

    DIRENTRY * pos;
    DIRS * empty_entry;
    
    pthread_rwlock_wrlock(&sb_lock);
    pthread_rwlock_wrlock(&inode_list_lock);
    pthread_rwlock_wrlock(&data_block_status_list_lock);
    
    file=fopen(path_to_fsfile,"rb+");
    
    if(file)
    {
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
        {
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
            
            
            
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
                
            {
                
                pos=search_name(file, temp->last_inode_used, temp->msg);

                if(pos&&pos->inode_ind)
                {
                    fprintf(stdout,"Inode ind %d\n",pos->inode_ind);
                    if((inode_list[pos->inode_ind].type==1)&&(inode_list[pos->inode_ind].reference_count==2))
                    {
                        fseek(file,data_block_status_list[ inode_list[temp->last_inode_used].data_block_offset[pos->i]].data_block_offset + sizeof(DIRS)*pos->j, SEEK_SET);
                        if((empty_entry=initialize_dir_entry()))
                        {
                            fwrite(empty_entry, sizeof(DIRS), 1, file);
                            dir_data_block_release(pos->inode_ind);
                            inode_list[pos->inode_ind].type=0;
                            inode_list[temp->last_inode_used].reference_count--;
                            
                            if(sb.remembered_inode > pos->inode_ind)
                                sb.remembered_inode=pos->inode_ind;
                            
                        
                            if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                            {
                        
                                empty_msg_from_svr_to_cli->error_code=0;
                                sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *Directory %s is deleted.\n",temp->userid,temp->client_pid,temp->msg);
                                empty_msg_from_svr_to_cli->more=0;
                                empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                empty_msg_from_svr_to_cli->ischangedirinst=0;
                                
                                if(stat(path_to_cli_fifo,&exist)==0)
                                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                free(empty_msg_from_svr_to_cli);
                        
                            }
                        }
                        else
                        {
                            fprintf(stderr,"Error on initializing dir entry.\n");
                        }

                        
                        
                    }
                    else
                    {
                        if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                        {
                            empty_msg_from_svr_to_cli->error_code=1;
                            sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *Can not delete directory :\n %s\n provided name %s is not directory or not empty.\n",temp->userid,temp->client_pid,temp->msg,temp->msg);
                            empty_msg_from_svr_to_cli->more=0;
                            empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                            empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                            if(stat(path_to_cli_fifo,&exist)==0)
                            write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                            free(empty_msg_from_svr_to_cli);
                        }
                        

                    }
                    
                    free(pos);
                    
                }
                else
                {
                    if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                    {
                        empty_msg_from_svr_to_cli->error_code=1;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->more=0;
                        sprintf(empty_msg_from_svr_to_cli->msg,"Provided directory %s is  not found.\n",temp->msg);
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                        if(stat(path_to_cli_fifo,&exist)==0)
                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                    
                        free(empty_msg_from_svr_to_cli);
                        
                    }
                    else
                    {
                        fprintf(stderr,"Error on allocating empty msg to client.\n");
                    }
                    
                }
                close(cli_fifo);
            }
            if(temp)
                free(temp);
            
        }
    }
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);

    return NULL;
    
    
}

void * create_file(void * args)
{
    MSGSVR * temp=(MSGSVR *) args;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    char empty_str[32];
    struct stat exist;
    
    int cli_fifo;
    FILE * file;
    
    DIRENTRY * pos;
    
    DIRS * empty_entry;
    
    empty_str[0]='\0';
    
    pthread_rwlock_wrlock(&sb_lock);
    pthread_rwlock_wrlock(&inode_list_lock);
    pthread_rwlock_wrlock(&data_block_status_list_lock);

    
    file=fopen(path_to_fsfile,"rb+");
    
    if(file)
    {
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
        {
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
            
            
            
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
                
            {
                
                pos=search_name(file, temp->last_inode_used, temp->msg);
                
                if(pos&&pos->inode_ind)
                {
                    fprintf(stdout,"Inode ind %d\n",pos->inode_ind);
                    if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                    {
                        
                        empty_msg_from_svr_to_cli->error_code=1;
                        sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *Can not create file :\n %s\n file/dir name already exists\n",temp->userid,temp->client_pid,temp->msg);
                        empty_msg_from_svr_to_cli->more=0;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                        if(stat(path_to_cli_fifo,&exist)==0)
                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                        free(empty_msg_from_svr_to_cli);
                        
                        
                    }
                    free(pos);
                    
                }
                else
                {
                    
                    fprintf(stdout,"last_inode_used:%d\n",temp->last_inode_used);
                    
                    if((pos=search_name(file, temp->last_inode_used, empty_str)))
                    {
                        fprintf(stdout,"pos i:%d j:%d\n",pos->i,pos->j);
                        fseek(file,data_block_status_list[ inode_list[temp->last_inode_used].data_block_offset[pos->i]].data_block_offset+sizeof(DIRS)*pos->j,SEEK_SET);
                        if((empty_entry=initialize_dir_entry()))
                        {
                            if((empty_entry->inode=next_free_inode()))
                            {
                                fprintf(stdout,"empty_entry inode: %d\n",empty_entry->inode);
                                
                                strcpy(empty_entry->name,temp->msg);
                                fwrite(empty_entry,sizeof(DIRS),1,file);
                                
                                dir_data_block_allocation(empty_entry->inode,2);
                                //dir_struct_creation(file, empty_entry->inode,temp->last_inode_used, 0);
                                if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                                {
                                    empty_msg_from_svr_to_cli->error_code=0;
                                    empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                    empty_msg_from_svr_to_cli->more=0;
                                    sprintf(empty_msg_from_svr_to_cli->msg,"New file %s is created.\n",empty_entry->name);
                                    empty_msg_from_svr_to_cli->ischangedirinst=0;
                                    
                                    if(stat(path_to_cli_fifo,&exist)==0)
                                    write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                    
                                    free(empty_msg_from_svr_to_cli);
                                }
                                else
                                {
                                    fprintf(stderr,"Error on allocating empty msg to client.\n");
                                }
                                
                                inode_list[temp->last_inode_used].reference_count++;
                            }
                            else
                            {
                                fprintf(stderr,"Error on new inode allocation.\n");
                            }
                            free(empty_entry);
                        }
                        else
                        {
                            fprintf(stderr,"Error in allocating directry entry.\n");
                        }
                        free(pos);
                    }
                    else
                    {
                        fprintf(stderr,"Error: Can not find any slot in present directory vacant");
                    }
                    
                }
                
                
                close(cli_fifo);
            }
            if(temp)
                free(temp);
            
        }
    }
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);

    return NULL;
    
    
}

void * delete_file(void * args)
{
    MSGSVR * temp=(MSGSVR *) args;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    struct stat exist;
    
    int cli_fifo;
    FILE * file;
    
    DIRENTRY * pos;
    DIRS * empty_entry;
    
    pthread_rwlock_wrlock(&sb_lock);
    pthread_rwlock_wrlock(&inode_list_lock);
    pthread_rwlock_wrlock(&data_block_status_list_lock);

    
    file=fopen(path_to_fsfile,"rb+");
    
    if(file)
    {
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
        {
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
            
            
            
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
                
            {
                
                pos=search_name(file, temp->last_inode_used, temp->msg);
                
                if(pos&&pos->inode_ind)
                {
                    fprintf(stdout,"Inode ind %d\n",pos->inode_ind);
                    if(inode_list[pos->inode_ind].type==2)
                    {
                        fseek(file, data_block_status_list[ inode_list[temp->last_inode_used].data_block_offset[pos->i]].data_block_offset+sizeof(DIRS)*pos->j, SEEK_SET);
                        if((empty_entry=initialize_dir_entry()))
                        {
                            fwrite(empty_entry, sizeof(DIRS), 1, file);
                            dir_data_block_release(pos->inode_ind);
                            inode_list[pos->inode_ind].type=0;
                            inode_list[temp->last_inode_used].reference_count--;
                            
                            if(sb.remembered_inode > pos->inode_ind)
                                sb.remembered_inode=pos->inode_ind;
                            
                            
                            if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                            {
                                
                                empty_msg_from_svr_to_cli->error_code=0;
                                sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *File %s is deleted.\n",temp->userid,temp->client_pid,temp->msg);
                                empty_msg_from_svr_to_cli->more=0;
                                empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                empty_msg_from_svr_to_cli->ischangedirinst=0;
                                
                                if(stat(path_to_cli_fifo,&exist)==0)
                                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                free(empty_msg_from_svr_to_cli);
                                
                            }
                        }
                        else
                        {
                            fprintf(stderr,"Error on initializing dir entry.\n");
                        }
                        
                        
                        
                    }
                    else
                    {
                        if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                        {
                            empty_msg_from_svr_to_cli->error_code=1;
                            sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *Can not delete file :\n %s\n provided name %s is not regular file.\n",temp->userid,temp->client_pid,temp->msg,temp->msg);
                            empty_msg_from_svr_to_cli->more=0;
                            empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                            empty_msg_from_svr_to_cli->ischangedirinst=0;
                            
                            if(stat(path_to_cli_fifo,&exist)==0)
                            write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                            free(empty_msg_from_svr_to_cli);
                        }
                        
                        
                    }
                    
                    free(pos);
                    
                }
                else
                {
                    if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                    {
                        empty_msg_from_svr_to_cli->error_code=1;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->more=0;
                        sprintf(empty_msg_from_svr_to_cli->msg,"Provided file %s is  not found.\n",temp->msg);
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                        if(stat(path_to_cli_fifo,&exist)==0)
                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                        
                        free(empty_msg_from_svr_to_cli);
                        
                    }
                    else
                    {
                        fprintf(stderr,"Error on allocating empty msg to client.\n");
                    }
                    
                }
                close(cli_fifo);
            }
            if(temp)
                free(temp);
            
        }
    }
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);
    
    return NULL;
    
    
}

void * rename_file_dir(void * args)
{
    MSGSVR * temp=(MSGSVR *) args;
    MSGCLI * empty_msg_from_svr_to_cli;
    char * path_to_cli_fifo;
    struct stat exist;
    
    char * present_name;
    char * new_name;
    
    int cli_fifo;
    FILE * file;
    
    DIRENTRY * pos;
    DIRENTRY * pos1;
    DIRS * empty_entry;
    
    pthread_rwlock_wrlock(&sb_lock);
    pthread_rwlock_wrlock(&inode_list_lock);
    pthread_rwlock_wrlock(&data_block_status_list_lock);

    
    file=fopen(path_to_fsfile,"rb+");
    
    if(file)
    {
        if((path_to_cli_fifo= (char *)malloc(sizeof(char)*PATHLENGTH)))
        {
            sprintf(path_to_cli_fifo,"/tmp/client.%d",temp->client_pid);
            
            
            
            cli_fifo=open(path_to_cli_fifo,O_WRONLY);
            if(cli_fifo)
                
            {
                present_name=strtok(temp->msg,"#");
                new_name=strtok(NULL,"#");
                
                fprintf(stdout,"Present name %s and New name %s\n", present_name,new_name);
                
                pos=search_name(file, temp->last_inode_used, present_name);
                pos1=search_name(file, temp->last_inode_used, new_name);
                
                if(pos&&pos->inode_ind&&!pos1)
                {
                    fprintf(stdout,"Inode ind %d\n",pos->inode_ind);
                    
                        fseek(file,data_block_status_list[ inode_list[temp->last_inode_used].data_block_offset[pos->i]].data_block_offset+sizeof(DIRS)*pos->j, SEEK_SET);
                        if((empty_entry=initialize_dir_entry()))
                        {
                            empty_entry->inode=pos->inode_ind;
                            strcpy(empty_entry->name,new_name);
                            fwrite(empty_entry, sizeof(DIRS), 1, file);
                            
                            if((empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                            {
                                
                                empty_msg_from_svr_to_cli->error_code=0;
                                sprintf(empty_msg_from_svr_to_cli->msg,"Your user id is %d using client ID %d\n *File %s is renamed to %s.\n",temp->userid,temp->client_pid,present_name,new_name);
                                empty_msg_from_svr_to_cli->more=0;
                                empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                                empty_msg_from_svr_to_cli->ischangedirinst=0;
                                
                                if(stat(path_to_cli_fifo,&exist)==0)
                                write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                                free(empty_msg_from_svr_to_cli);
                                
                            }
                        }
                        else
                        {
                            fprintf(stderr,"Error on initializing dir entry.\n");
                        }
                        
                        
                        
                    
                    
                    free(pos);
                    
                }
                else
                {
                    if(!pos1&&
                       (empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                    {
                        empty_msg_from_svr_to_cli->error_code=1;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->more=0;
                        sprintf(empty_msg_from_svr_to_cli->msg,"Provided file/directory %s is  not found.\n",temp->msg);
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                        if(stat(path_to_cli_fifo,&exist)==0)
                        write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                        
                        free(empty_msg_from_svr_to_cli);
                        
                    }
                    else if(pos1&&
                       (empty_msg_from_svr_to_cli=initialize_empty_msg_to_cli()))
                    {
                        empty_msg_from_svr_to_cli->error_code=1;
                        empty_msg_from_svr_to_cli->last_inode_used=temp->last_inode_used;
                        empty_msg_from_svr_to_cli->more=0;
                        sprintf(empty_msg_from_svr_to_cli->msg,"Provided new file/directory %s is  already present.\n",temp->msg);
                        empty_msg_from_svr_to_cli->ischangedirinst=0;
                        
                        if(stat(path_to_cli_fifo,&exist)==0)
                            write(cli_fifo,empty_msg_from_svr_to_cli,sizeof(MSGCLI));
                        
                        free(empty_msg_from_svr_to_cli);
                        
                    }
                    else
                    {
                        fprintf(stderr,"Error on allocating empty msg to client.\n");
                    }
                    
                }
                close(cli_fifo);
            }
            if(temp)
                free(temp);
            
        }
    }
    
    if(file)
        fclose(file);
    
    pthread_rwlock_unlock(&data_block_status_list_lock);
    pthread_rwlock_unlock(&inode_list_lock);
    pthread_rwlock_unlock(&sb_lock);

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
                case 4:
                    pthread_create(&tid,NULL,make_dir,empty_msg_to_server);
                    break;
                case 5:
                    pthread_create(&tid, NULL, remove_dir, empty_msg_to_server);
                    break;
                case 6:
                    pthread_create(&tid,NULL,create_file,empty_msg_to_server);
                    break;
                case 7:
                    pthread_create(&tid,NULL,delete_file,empty_msg_to_server);
                    break;
                case 8:
                    pthread_create(&tid,NULL,rename_file_dir,empty_msg_to_server);
                    break;
                case 99:
                    pthread_create(&tid,NULL,client_ternimation,empty_msg_to_server);
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
        pthread_rwlock_init(&inode_list_lock,NULL);
        pthread_rwlock_init(&data_block_status_list_lock, NULL);
        pthread_rwlock_init(&sb_lock, NULL);

        readfromfifo();
        
    }
}

void fs_file_creation()
{
    int i;
    fsfile=fopen(path_to_fsfile,"wb+");
    if(fsfile)
    {
        sb.blocksize=blocksize;
        sb.filesystemsize=size_in_bytes;
        sb.remembered_data_block=0;
        sb.remembered_inode=1;
        sb.no_of_inode=no_of_inodes;
        sb.no_of_data_blocks=no_of_datablocks;
        sb.no_of_directory_entry=no_of_directory_entry;
        fwrite(&sb,sizeof(SB),1,fsfile);
        inode_list=(INODE *)malloc(sizeof(INODE)*no_of_inodes);
        if(inode_list)
        {
            fprintf(stdout,"Size of Super block : %lu\n",sizeof(SB));
            fprintf(stdout,"Size of empty inode : %lu\n",sizeof(empty_inode));
            empty_inode.accessed=0;
            empty_inode.created=0;
            for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
                empty_inode.data_block_offset[i]=-1;
            empty_inode.lock=0;
            empty_inode.modified=0;
            empty_inode.type=0;
            for(i=0;i<no_of_inodes;i++)
                fwrite(&empty_inode,sizeof(empty_inode),1,fsfile);
            
            
            free(inode_list);
            
        }
        else
        {
            fprintf(stderr,"Error in inode list memory allocation.\n");
           // exit(1);
        }
        fprintf(stdout,"Size of data block status : %lu \n",sizeof(DBS));
        data_block_status_list=(DBS *)malloc(sizeof(DBS)*no_of_datablocks);
        fprintf(stdout,"Ftell %lld\n",ftello(fsfile));
        if(data_block_status_list)
        {
            for(i=0;i<no_of_datablocks;i++)
            {
                data_block_status_list[i].empty=1;
                data_block_status_list[i].lock=0;
                data_block_status_list[i].data_block_offset=sizeof(SB)+sizeof(INODE)*no_of_inodes+sizeof(DBS)*no_of_datablocks+i*blocksize;
                //fprintf(stdout,"Data block  offset calculated before write:%lld\n",data_block_status_list[i].data_block_offset);
                fwrite((data_block_status_list+i),sizeof(DBS),1,fsfile);
            }
            free(data_block_status_list);
        }
        else
        {
            fprintf(stderr,"Error in data block status list.\n");
           // exit(2);
        }
        
        for(i=0;i<no_of_datablocks;i++)
            fwrite(&c,sizeof(c),blocksize,fsfile);
        
        fprintf(stdout,"Unused bytes : %llu\n",size_in_bytes - ftello(fsfile));
        

        fclose(fsfile);
    }
    else
    {
        fprintf(stderr,"Error with filesystem file opening.\n");
    }
    if(fsfile)
        fclose(fsfile);
        
}

void root_inode_creation()
{
    int i/*,j*/;
   // DIRS * temp;
    inode_list[1].lock=1;
    inode_list[1].created=time(NULL);
    inode_list[1].accessed=time(NULL);
    inode_list[1].modified=time(NULL);

    inode_list[1].type=1;
    for(i=0;(i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE)
                && (sb.remembered_data_block<sb.no_of_data_blocks);
                                            i++, sb.remembered_data_block++)
    {
        data_block_status_list[sb.remembered_data_block].lock=1;
        data_block_status_list[sb.remembered_data_block].empty=0;
        
        inode_list[1].data_block_offset[i]=sb.remembered_data_block;
        
        data_block_status_list[sb.remembered_data_block].lock=0;
    }
    inode_list[1].lock=0;
    
   /* for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
    {
        fprintf(stdout,"Data blocks assigned to root inode: %lld\n",inode_list[1].data_block_offset[i]);
    }*/
    
    sb.remembered_inode++;
    
    dir_struct_creation(fsfile,1,1,1);
/*    for(i=0;i<NO_OF_DATA_BLOCK_OFFSET_IN_INODE;i++)
    {
        fprintf(stdout,"Inode data off set %d and data block %lld \n",inode_list[1].data_block_offset[i],data_block_status_list[inode_list[1].data_block_offset[i]].data_block_offset);
        fseek(fsfile,data_block_status_list[ inode_list[1].data_block_offset[i]].data_block_offset,SEEK_SET);
        for(j=0;j<sb.no_of_directory_entry;j++)
        if((temp=initialize_dir_entry()))
        {
            fread(temp,sizeof(DIRS),1,fsfile);
            fprintf(stdout,"Inode number %d and directory name %s\n",
                                                    temp->inode,temp->name);
            free(temp);
        }
        
    }
 */
    
}
void fs_file_open()
{
    int i;
    fsfile=fopen(path_to_fsfile,"rb+");
    if(fsfile)
    {
        fread(&sb,sizeof(SB),1,fsfile);
        fprintf(stdout,"blocksize in filesystem : %lu and blocksize provided in command line : %lu\n",sb.blocksize,blocksize);
        if(sb.blocksize!=blocksize)
            fprintf(stderr,"Blocksize in command line is not matching, proceeding with filesystem blocksize\n");
        fprintf(stdout,"Filesystem size : %lu and filesystem size provided in command line  : %lu\n",sb.filesystemsize,size_in_bytes);
        if(sb.filesystemsize!=size_in_bytes)
            fprintf(stderr,"Filesystem size provided in command line is not matching with filesystem size, proceeding with filesystem file size\n");
        
        inode_list=(INODE *)malloc(sizeof(INODE)*sb.no_of_inode);
        
        if(inode_list)
        {
            for(i=0;i<sb.no_of_inode;i++)
                fread((inode_list+i),sizeof(INODE),1,fsfile);
            
        }
        else
        {
            fprintf(stderr,"Problem with inode list allocation in memory.\n");
            //exit(6);
        }
        
        data_block_status_list=(DBS *)malloc(sizeof(DBS)*sb.no_of_data_blocks);
        if(data_block_status_list)
        {
            for(i=0;i<sb.no_of_data_blocks;i++)
            {
                fread((data_block_status_list+i),sizeof(DBS),1,fsfile);
              //  fprintf(stdout,"Data block offset :%lld\n",data_block_status_list[i].data_block_offset);
            }
        }
        else
        {
            fprintf(stderr,"Problem with data block status list allocation in memory.\n");
            //exit(7);
            
        }
            
        
        if(sb.remembered_inode==1)
        {
            fprintf(stdout,"Setting up root inode...\n");
            root_inode_creation();
        }
        
    }
    else
    {
        fprintf(stderr,"Peoblem with filesystem file opening.\n");
        //exit(3);
    }
    if(fsfile)
        fclose(fsfile);
}


int main(int argc, const char * argv[]) {
    // insert code here...
    
    signal(SIGINT,cleanup);
    
    if(argc!=5)
        printf("usage:%s <size of filesystem in MB> <blocksize in bytes> <path to filesystem file > <path to FIFO file>\n",argv[0]);
    else
    {
        size_in_MB=atoi(argv[1]);
        size_in_bytes=size_in_MB<<20;
        blocksize=atoi(argv[2]);
        strcpy(path_to_fsfile,argv[3]);
        strcpy(path_to_fifo,argv[4]);
        
        no_of_inodes=(int)(size_in_bytes-sizeof(SB))/(sizeof(INODE)+sizeof(DBS)+blocksize);
        no_of_datablocks=no_of_inodes;
        no_of_directory_entry=(int)blocksize/sizeof(DIRS);
        fprintf(stdout,"No of inode possible : %d\n",no_of_inodes);
        if(stat(path_to_fsfile,&test)==0)
        {
            fprintf(stdout,"Given filesystem file exists\n");
            fprintf(stdout,"Opening filesystem file. \n");
            fs_file_open();
            
        }
        else
        {
            fprintf(stderr,"Given filesystem file does not exist : %s\n",strerror(errno));
            fprintf(stdout,"New filesystem file is going to be created.\n");
            fs_file_creation();
            fprintf(stdout,"Filesystem file creation is successful.\n");
            fprintf(stdout,"Opening the filesystem file.\n");
            fs_file_open();
            
        }
        

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
            fprintf(stderr,"Something went wrong with server FIFO: %s\n",strerror(errno));
            fprintf(stdout,"Creating new FIFO with given path\n");
            
            if(mkfifo(path_to_fifo,MKFIFO_FILE_MODE))
                fprintf(stderr,"mkfifo error :%s\n",strerror(errno));
            else
            start_server();
        }
    }
    
    
    return 0;
}
