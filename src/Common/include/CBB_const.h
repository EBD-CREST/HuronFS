#ifndef CBB_CONST_H_
#define CBB_CONST_H_

const size_t KB=1000;
const size_t MB=KB*1000;
const size_t GB=MB*1000;

const int MASTER_PORT = 9000; 
const int MASTER_CONN_PORT = 8001; 
const int IONODE_PORT = 8000; 
//const int CLIENT_PORT = 8001;
//const int IO_CLIENT_PORT = 8002;

const int MEMORY = 10000; 
const int MAX_BLOCK_NUMBER = 10; 
const int MAX_FILE_NUMBER = 1000; 
const int MAX_QUEUE = 100; 
//const int BLOCK_SIZE = 1000; 
const int MAX_NODE_NUMBER = 10; 
const int LENGTH_OF_LISTEN_QUEUE = 10; 
const int MAX_SERVER_BUFFER = 1000; 
const int MAX_COMMAND_SIZE = 1000; 
const int MAX_CONNECT_TIME = 10;
const int MAX_QUERY_LENGTH = 100; 
const int MAX_FILE = 10000;
const int INIT_FD = 100000;
const int CONNECT_WAIT_TIME=1000;
const size_t BLOCK_SIZE = 64*MB;
const int NO_ERROR=0;
const size_t STREAM_BUFFER_SIZE = BLOCK_SIZE;

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

const int SERVER_SHUT_DOWN = 20; 
const int I_AM_SHUT_DOWN = 21;

const int TOO_MANY_FILES =50;
const int FILE_NOT_FOUND = 51;
const int UNKNOWN_ERROR = 52;
const int NO_SUCH_FILE = 53;


const int OUT_OF_RANGE=54;//"out of range\n"; 

#endif
