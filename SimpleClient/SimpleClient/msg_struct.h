//
//  msg_struct.h
//  SimpleClient
//
//  Created by Sankarsan Seal on 24/07/16.
//  Copyright © 2016 Sankarsan Seal. All rights reserved.
//

#ifndef msg_struct_h
#define msg_struct_h
#define MKFIFO_FILE_MODE (S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP|S_IROTH)
#define PATHLENGTH 1024

typedef struct msg_to_svr
{
    int instruct_code;
    int sub_instruction;
    pid_t client_pid;
    int userid;
    int last_inode_used;
    char msg[PATHLENGTH];
} MSGSVR;

typedef struct msg_to_client
{
    int error_code;
    int last_inode_used;
    int more;
    char msg[PATHLENGTH];
} MSGCLI;

#endif /* msg_struct_h */

