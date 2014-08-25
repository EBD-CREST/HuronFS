#ifndef IO_CONST_H_
#define IO_CONST_H_

const int MASTER_PORT = 9000; 
const int MASTER_CONN_PORT = 9001; 
const int IONODE_PORT = 8000; 

const int MEMORY = 10000; 
const int MAX_BLOCK_NUMBER = 10; 
const int MAX_FILE_NUMBER = 1000; 
const int MAX_QUEUE = 100; 
const int BLOCK_SIZE = 1000; 
const int MAX_NODE_NUMBER = 10; 
const int LENGTH_OF_LISTEN_QUEUE = 10; 
const int MAX_SERVER_BUFFER = 1000; 
const int MAX_COMMAND_SIZE = 1000; 
const int MAX_CONNECT_TIME = 10;
const int MAX_QUERY_LENGTH = 100; 

const bool CLEAN = false;
const bool DIRTY = true;
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

const int SERVER_SHUT_DOWN = 20; 
const int I_AM_SHUT_DOWN = 21;

const int TOO_MANY_FILES =50;
const int FILE_NOT_FOUND = 51;
const int UNKNOWN_ERROR = 52;

#endif
