#ifndef CBB_CONST_H_
#define CBB_CONST_H_

const size_t KB=1000;
const size_t MB=KB*1000;
const size_t GB=MB*1000;

const size_t MAX_FILE_SIZE = 10*GB;
const int MASTER_PORT = 9000; 
const int MASTER_CONN_PORT = 7001; 
const int IONODE_PORT = 7000; 
const int MAX_IONODE = 1000;
const int MAX_BASIC_MESSAGE_SIZE = 1*MB;
//const int CLIENT_PORT = 8001;
//const int IO_CLIENT_PORT = 8002;
const int SERVER_THREAD_NUM = 1;
const int CLIENT_THREAD_NUM = 1;

const int MASTER_QUEUE_NUM=SERVER_THREAD_NUM+1;
const int IONODE_QUEUE_NUM=SERVER_THREAD_NUM+1;
const int CLIENT_QUEUE_NUM=CLIENT_THREAD_NUM;

const int HEART_BEAT_QUEUE_NUM = SERVER_THREAD_NUM;
const int DATA_SYNC_QUEUE_NUM = SERVER_THREAD_NUM;

const int MEMORY = 10000; 
const int MAX_BLOCK_NUMBER = 1000; 
const int MAX_FILE_NUMBER = 1000000; 
const int MAX_QUEUE = 100; 
//const int BLOCK_SIZE = 1000; 
const int MAX_NODE_NUMBER = 10; 
const int LENGTH_OF_LISTEN_QUEUE = 1; 
const int MAX_SERVER_BUFFER = 1000; 
const int MAX_COMMAND_SIZE = 1000; 
const int MAX_CONNECT_TIME = 10;
const int MAX_QUERY_LENGTH = 100; 
const int MAX_FILE = 10000;
const int INIT_FD = 100000;
const int CONNECT_WAIT_TIME=1000;
const size_t BLOCK_SIZE = 5*MB;
const int NO_ERROR=0;
const size_t STREAM_BUFFER_SIZE = 2*BLOCK_SIZE;
const size_t MAX_TRANSFER_SIZE = STREAM_BUFFER_SIZE;

const bool CLEAN = false;
const bool DIRTY = true;
const bool VALID = true;
const bool INVALID = false;
const int FAILURE = -1;
const int SUCCESS = 0;

const int CLOSE_PIPE = 0;
const int REGIST = 1; 
const int UNREGIST = 2;
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
const int SOCKET_KILLED = 58;
const int EXISTING = 1;
const int NOT_EXIST = 0;

const int OUT_OF_RANGE=100;

const int EXTERNAL = 0;
const int INTERNAL = -1;
const int RENAMED = 1;
const int MYSELF = -1;

const int KEEP_ALIVE = 0;
const int NOT_KEEP_ALIVE =1;

const int SEND = 0;
const int RECV_EXTENDED_MESSAGE = 1;

const bool STARTED = true;
const bool UNSTARTED = false;
const bool SET = true;
const bool STREAM_BUFFER_FLAG = true;

const int CBB_REMOTE_WRITE_BACK = 0;
const int HEART_BEAT_INTERVAL=1000;

const int NUM_OF_REPLICA = 2;

const int MAIN_REPLICA=0;
const int SUB_REPLICA=1;

const int DATA_SYNC_INIT=0;
const int DATA_SYNC_WRITE=1;
#endif
