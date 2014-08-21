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

const int SUCCESS = 0; 
const int REGIST = 1; 
const int UNREGIST = 2;
const int BUFFER_FILE = 3; 
const int UNRECOGNISTED=9; 

const int SERVER_SHUT_DOWN = 10; 

const int PRINT_NODE_INFO = 20; 
const int VIEW_FILE_INFO = 21; 

const int OPEN_FILE = 40; 

#endif
