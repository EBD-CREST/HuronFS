/*
 * Master.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define Master class in I/O burst buffer
 */
#ifndef MASTER_H_
#define MASTER_H_
#include "Node_info.h"

class Master
{
	//API
public:
	Master();
	Master();
	~Master();

private:
	std::vector<class node_info> IOnodes;
	static int number_node;
	std::queue<class block> buffer_queue;
};

#endif



