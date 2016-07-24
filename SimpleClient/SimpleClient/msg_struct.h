//
//  msg_struct.h
//  SimpleClient
//
//  Created by Sankarsan Seal on 24/07/16.
//  Copyright © 2016 Sankarsan Seal. All rights reserved.
//

#ifndef msg_struct_h
#define msg_struct_h

typedef struct msg_to_svr
{
    int instruct_code;
    pid_t client_pid;
    char msg[512];
} MSGSVR;

#endif /* msg_struct_h */

