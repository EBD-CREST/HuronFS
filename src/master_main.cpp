#include <stdlib.h>
#include <stdio.h>

#include "include/Master.h"

int main(int argc, char **argv)
{
	Master master; 
	master.start_server(); 
	master.stop_server(); 
	return 0; 
}
