#include "CBB_basic.h"
using namespace CBB::Client;
using namespace CBB::Common;

block_info::block_info(off64_t start_point,
		       size_t size):
	start_point(start_point),
	size(size)
{}

file_meta::file_meta(ssize_t remote_file_no,
		     size_t block_size,
		     const struct stat* file_stat,
		     SCBB* corresponding_SCBB):
	remote_file_no(remote_file_no),
	open_count(0),
	block_size(block_size),
	file_stat(*file_stat),
	opened_fd(),
	corresponding_SCBB(corresponding_SCBB),
	it(nullptr)
{}

opened_file_info::
opened_file_info(int 		fd,
		int 		flag,
		file_meta* 	file_meta_p):
	current_point(0),
	fd(fd),
	flag(flag),
	file_meta_p(file_meta_p)
{
	++file_meta_p->open_count;
}

opened_file_info::opened_file_info():
	current_point(0),
	fd(0),
	flag(-1),
	file_meta_p(nullptr),
	IOnode_list_cache(),
	block_list()
{}

opened_file_info::~opened_file_info()
{
	if(nullptr != file_meta_p && 0 == --file_meta_p->open_count)
	{
		delete file_meta_p;
	}
}

opened_file_info::opened_file_info(const opened_file_info& src):
	current_point(src.current_point),
	fd(src.fd),
	flag(src.flag),
	file_meta_p(src.file_meta_p),
	IOnode_list_cache(src.IOnode_list_cache),
	block_list(src.block_list)
{
	++file_meta_p->open_count;
}

SCBB::SCBB(int id,
	   comm_handle_t handle):
	IOnode_list(),
	id(id),
	master_handle(*handle)
{}

comm_handle_t SCBB::get_IOnode_fd(ssize_t IOnode_id)	
{
	return &IOnode_list.at(IOnode_id);
}
