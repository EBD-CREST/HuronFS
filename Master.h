/*
 * Master.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define Master class in I/O burst buffer
 */
#ifndef MASTER_H_
#define MASTER_H_

#include <map>
#include <vector>
#include <unistd.h>

class Master
{
	//API
public:
	Master();
	Master();
	~Master();
private:
	//map file_no, vector<start_point>
	typedef std::map<int, std::vector<std::size_t> > block_info; 
	class node_info
	{
	public:
		node_info(std::string ip, std::size_t avaliable_memory); 
		~node_info();
		
		std::string ip; 
		block_info blocks; 
		std::size_t avaliable_memory; 
		std::size_t total_memory; 
	}

private:
	std::vector<class node_info> IOnodes;
	static int number_node;
	std::queue<class block> buffer_queue;
	
};

#endif
