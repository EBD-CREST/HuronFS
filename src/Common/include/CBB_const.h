/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef CBB_CONST_H_
#define CBB_CONST_H_

const size_t KB=1000;
const size_t MB=KB*1000;
const size_t GB=MB*1000;

//the port number used by master
const int MASTER_SOCKET_PORT = 9000; 
//the port number used to Connect master in each IOnode
const int MASTER_CONN_PORT = 7001; 
//the port number used by IONode
const int IONODE_SOCKET_PORT = 7000; 
//the port number used on Master for exchanging URI in CCI
const int MASTER_URI_EXCHANGE_PORT = 9001;
//the port number used on IOnode for exchanging URI in CCI
const int IONODE_URI_EXCHANGE_PORT = 9002;

#ifdef CCI
//const int URI_EXCHANGE_PORT = 9001;
const int MASTER_PORT = MASTER_URI_EXCHANGE_PORT; 
//the port number used by IONode
const int IONODE_PORT = IONODE_URI_EXCHANGE_PORT; 
#else
const int MASTER_PORT = MASTER_SOCKET_PORT; 
//the port number used by IONode
const int IONODE_PORT = IONODE_SOCKET_PORT; 
#endif

//the maximum IOnode under each master
const int MAX_IONODE = 1000;
//the size of basic message in communication
const int MAX_BASIC_MESSAGE_SIZE = 4*KB;
//the size of extended message located in IO task
const int MAX_EXTENDED_MESSAGE_SIZE=5*MB;
//the size of files in a directory
const int MAX_DIR_FILE_SIZE=MAX_EXTENDED_MESSAGE_SIZE;
//the max retry for communication
const int CBB_COMM_RETRY = 10;

const int RECEIVER_ID_OFF = 0;
const int SENDER_ID_OFF = RECEIVER_ID_OFF + sizeof(int);
const int MESSAGE_SIZE_OFF = SENDER_ID_OFF + sizeof(int);
const int EXTENDED_SIZE_OFF = MESSAGE_SIZE_OFF + sizeof(size_t);
const int MESSAGE_META_OFF = EXTENDED_SIZE_OFF + sizeof(size_t);
const int RECV_MESSAGE_OFF = MESSAGE_META_OFF - sizeof(int);
//const int CLIENT_PORT = 8001;
//const int IO_CLIENT_PORT = 8002;

//the number of handler threads in server
const int SERVER_THREAD_NUM = 1;
//the number of handler threads in client
const int CLIENT_THREAD_NUM = 10;

//the number of queue used to send heart beat in Master= 1
const int HEART_BEAT_THREAD_NUM = 1;
//the number of queue used to send data synchronization in IOnode= 1
const int DATA_SYNC_THREAD_NUM = 1;
//the number of queue used to send heart beat in Master= 1
const int CONNECTION_THREAD_NUM=1;

const int IONODE_PRE_ALLOCATE_MEMORY_COUNT=100;

#ifdef SINGLE_THREAD
const int CLIENT_PRE_ALLOCATE_MEMORY_COUNT=10;
#else
const int CLIENT_PRE_ALLOCATE_MEMORY_COUNT=0;
#endif

//the number of communication queues used in master
//the number = # of handler threads in server + # of queue used to send heart beat
const int MASTER_QUEUE_NUM=SERVER_THREAD_NUM+HEART_BEAT_THREAD_NUM+CONNECTION_THREAD_NUM;
//the index of connection queue 
const int CONNECTION_QUEUE_NUM=SERVER_THREAD_NUM;
//the index of heart beat queue in Master
const int MASTER_HEART_BEAT_QUEUE_NUM=CONNECTION_QUEUE_NUM+CONNECTION_THREAD_NUM;
//the number of communication queues used in IOnode 
//the number = # of handler threads in server + # of remote handler
const int IONODE_QUEUE_NUM=SERVER_THREAD_NUM+DATA_SYNC_THREAD_NUM+CONNECTION_THREAD_NUM;
//the index of data sync queue in IOnode
const int IONODE_DATA_SYNC_QUEUE_NUM=CONNECTION_QUEUE_NUM+CONNECTION_THREAD_NUM;
//the number of communication queues used in client
//the number = # of handler threads in client 
const int CLIENT_QUEUE_NUM=CLIENT_THREAD_NUM;

//the total memory available in each IOnode
//this is the default value
const int MEMORY = 10000; 
//the maximum number of block for each file
//not used
const int MAX_BLOCK_NUMBER = 1000; 
//the size of block used by all the file
//important
const size_t BLOCK_SIZE = 5*MB;
//the maximum file size supported;
//the number = block size * max block number
//not used
const size_t MAX_FILE_SIZE = BLOCK_SIZE*MAX_BLOCK_NUMBER;
//the maximum file number supported;
const int MAX_FILE_NUMBER = 1000000; 
//the max queue used in server listen as "backlog"
const int MAX_QUEUE = 100; 
//const int BLOCK_SIZE = 1000; 
//const int MAX_NODE_NUMBER = 10; 


//the number of listening queue used by epoll
//the total number got in each poll
const int LENGTH_OF_LISTEN_QUEUE = 1; 
//const int MAX_SERVER_BUFFER = 1000; 
//const int MAX_COMMAND_SIZE = 1000; 
//const int MAX_QUERY_LENGTH = 100; 

//the maximum file number can be opened in Client
const int MAX_FILE = 10000;


//the initial fd, used in Client
//to avoid conflicts with normal fd, please set a large number
//mainly used in preload
const int INIT_FD = 1000;
//the maximum retry in issuing connection
const int MAX_CONNECT_TIME = 10;
//wait time between each connection retry
const int CONNECT_WAIT_TIME=1000;
//the size of IO buffer in each Client
//used in buffered IO
//const size_t STREAM_BUFFER_SIZE = BLOCK_SIZE;
const size_t STREAM_BUFFER_SIZE = 100*MB;
//the maximum size of data in each transfer 
const size_t MAX_TRANSFER_SIZE = STREAM_BUFFER_SIZE;

//all the flags
const bool CLEAN = false;
const bool DIRTY = true;
const bool VALID = true;
const bool INVALID = false;
const int FAILURE = -1;
const int SUCCESS = 0;
//no error
//used as a return value in socket error report
const int NO_ERROR=0;
const bool SET = true;

//command operations
const int CLOSE_PIPE = 0;
const int REGISTER = 1; 
const int UNREGISTER = 2;
const int PRINT_NODE_INFO = 3; 
const int GET_FILE_INFO = 4; 
const int OPEN_FILE = 5;
const int READ_FILE = 6;
const int WRITE_FILE = 7;
const int FLUSH_FILE = 8;
const int CLOSE_FILE = 9;
const int UNRECOGNISTED=10;
const int GET_FILE_META = 11;
const int APPEND_BLOCK = 12;
const int GET_ATTR = 13;
const int READ_DIR =14;
const int RM_DIR = 15;
const int UNLINK = 16;
const int ACCESS = 17;
const int MKDIR = 18;
const int RENAME = 19;
const int NEW_CLIENT = 20;
const int CLOSE_CLIENT = 21;
const int TRUNCATE = 22;
const int RENAME_MIGRATING = 23;
const int HEART_BEAT = 24;
const int NODE_FAILURE = 25;
const int OPEN_FILE_RECOVERY = 26;
const int DATA_SYNC = 27;
const int NEW_IONODE = 28;
const int PROMOTED_TO_PRIMARY_REPLICA = 29;
const int REPLACE_REPLICA = 30;
const int REMOVE_IONODE = 31;
const int IONODE_FAILURE = 32;

const int SERVER_SHUT_DOWN = 40; 
const int I_AM_SHUT_DOWN = 41;

const int TOO_MANY_FILES =50;
const int FILE_NOT_FOUND = 51;
const int UNKNOWN_ERROR = 52;
const int NO_SUCH_FILE = 53;
const int OUT_OF_DATE = 54;
const int SEND_META = 55;
const int RECV_META = 56;
const int NO_NEED_META = 57;
const int EXISTING = 1;
const int NOT_EXIST = 0;

const int OUT_OF_RANGE=100;

//external flags
//used in Master in file rename/symblic link
//internal means file stored in this master
//external means file stored in other master, the file in this Master is a link 
const int EXTERNAL = 0;
const int INTERNAL = -1;
//rename means file stored in other master, the file in this Master is a renamed file
const int RENAMED = 1;
const int MYSELF = -1;

//flags used to control communication threads
const int KEEP_ALIVE = 0;
const int NOT_KEEP_ALIVE =1;

//connection errors
const int SOCKET_KILLED = 1;
const int CONNECTION_SETUP_FAILED= 2;
//flags
//const int SEND = 0;
//const int RECV_EXTENDED_MESSAGE = 1;

//flag started
const bool STARTED = true;
const bool UNSTARTED = false;
//const bool STREAM_BUFFER_FLAG = false;

//mode in write back
const int CBB_REMOTE_WRITE_BACK = 0;
//the heart beat check interval
const int HEART_BEAT_INTERVAL=1000;

//number of replication
//the total number of copy=number of replication + primary replica
const int NUM_OF_REPLICA = 0;

//main replica flag
const int MAIN_REPLICA=0;
const int SUB_REPLICA=1;

//modes in data synchronization
const int DATA_SYNC_INIT=0;
const int DATA_SYNC_WRITE=1;

const int URI_MAX = 1000;
const int TIMEOUT_SEC=1;
const bool SETUP_CONNECTION = true;
const bool NORMAL_IO=false;

const int RMA_READ=0;
const int RMA_WRITE=1;
const int RMA_DIR=2;

//the len of ip
const int IP_STRING_LEN=20;

//write back status
const int IDLE=0;
const int ON_GOING=1;

const int WRITEBACK_SIZE=10;

const int PRINT_DELAY=100;
#endif
