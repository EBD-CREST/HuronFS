/*
 * IOnode.c
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include "IOnode.h"
#include <malloc.h>

IOnode::block::block(unsigned long size, int block_id):size(size),block_id(block_id),data(NULL)
{
	data=malloc(size);
	if( NULL == data)
	{
		std::bad_alloc e("out of memory");
		throw e;
	}
	return;
}

IOnode::block::~block()
{
	free(size);
}

IOnode::block::block(const block & src):size(src.size),block_id(src.block_id),data(src.data){};

IOnode::IOnode(std::string my_ip, std::string master_ip):ip(my_ip),
	node_id(init(master_ip)),
	{};

IOnode::~IOnode()
{

	
