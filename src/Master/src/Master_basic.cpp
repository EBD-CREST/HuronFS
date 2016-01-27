#include "Master_basic.h"

using namespace CBB::Master;

open_file_info::open_file_info(ssize_t fileno,
		size_t block_size,
		const node_id_pool_t& IOnodes,
		int flag,
		Master_file_stat* file_stat):
	file_no(fileno),
	block_list(),
	IOnodes_set(IOnodes), 
	main_node_id(-1),
	block_size(block_size),
	open_count(1),
	flag(flag),
	file_status(file_stat)
{}

open_file_info::open_file_info(ssize_t fileno,
		int flag,
		Master_file_stat* file_stat):
	file_no(fileno),
	block_list(),
	IOnodes_set(), 
	main_node_id(-1),
	block_size(0),
	open_count(1),
	flag(flag),
	file_status(file_stat)
{}

/*void open_file_info::_set_nodes_pool()
{
	for(::const_iterator it=p_node.begin();
			it != p_node.end();++it)
	{
		nodes.insert(it->second);
	}
	return;
}*/

Master_file_stat::Master_file_stat(const struct stat& file_stat, 
		const std::string& filename,
		int exist_flag):
	status(file_stat),
	name(filename),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(exist_flag),
	full_path(nullptr),
	opened_file_info(nullptr)
{}

Master_file_stat::Master_file_stat(const struct stat& file_stat, 
		const std::string& filename,
		open_file_info* opened_file_info,
		int exist_flag):
	status(file_stat),
	name(filename),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(exist_flag),
	full_path(nullptr),
	opened_file_info(opened_file_info)
{}

Master_file_stat::Master_file_stat():
	status(),
	name(),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(),
	full_path(nullptr),
	opened_file_info()
{}

node_info::node_info(ssize_t id,
		const std::string& ip,
		size_t total_memory,
		int socket):
	socket(socket),
	node_id(id),
	ip(ip), 
	stored_files(file_no_pool_t()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory)
{}
