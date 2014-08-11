/*
 * IOnode.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define I/O node class in I/O burst buffer
 */


#ifndef IONODE_H_
#define IONODE_H_

class IOnode
{
//nested class
private:

	struct block
	{
		block(unsigned long size, int block_id) throw(std::bad_alloc);
		~block();
		block(const block&);
		unsigned long size;
		void *data;
		const int block_id;
	};
//API
public:
	IOnode(std::string my_ip, std::string master_ip);
	~IOnode();
	int insert_block(unsigned long size);
	int delete_block();

public:
	const std::string ip;
	const int node_id;

private:
	//map : block_id , block
	std::map<int,std::vector<class block>> _blocks;
	unsigned int _current_block_number;
	static unsigned int MAX_BLOCK_NUMBER;
	unsigned long _memory;
	int init(std::string master_ip);
};

#endif
