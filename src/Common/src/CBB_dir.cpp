#include "CBB_dir.h"

using namespace CBB::Common;

_basic_item::_basic_item():
	status(),
	name()
{}

_basic_item::_basic_item(const struct stat& file_status, const std::string& filename):
	status(file_status),
	name(filename)
{}

_basic_item::~_basic_item()
{}

_basic_dir::_basic_dir():
	_basic_item(),
	items()
{}

_basic_dir::_basic_dir(const struct stat& file_status, const std::string& filename):
	_basic_item(file_status, filename),
	items()
{}

_basic_dir::~_basic_dir()
{
	for(items_set_t::iterator it=items.begin();
			it!=items.end();++it)
	{
		delete *it;
	}
}

_basic_dir::items_set_t::iterator _basic_dir::add_item(_basic_item* item)
{
	CBB_rwlocker::wr_lock();
	items_set_t::iterator it=items.insert(item).first;
	CBB_rwlocker::unlock();
	return it;
}

bool _basic_dir::remove_item(_basic_item* item)
{
	CBB_rwlocker::wr_lock();
	delete item;
	int ret=items.erase(item);
	CBB_rwlocker::unlock();
	return ret;

}

bool _basic_dir::remove_item(const std::string& file_name)
{
	CBB_rwlocker::rd_lock();
	for(items_set_t::iterator it=items.begin();
			it!=items.end();++it)
	{
		if(0 == file_name.compare((*it)->get_filename()))
		{
			CBB_rwlocker::unlock();
			return remove_item(it);
		}
	}
	CBB_rwlocker::unlock();
	return FAILURE;
}

bool _basic_dir::remove_item(const items_set_t::iterator& it)
{
	CBB_rwlocker::wr_lock();
	delete *it;
	items.erase(it);
	CBB_rwlocker::unlock();
	return SUCCESS;
}

_basic_dir::_basic_item* find_by_name(const std::string filename, _basic_dir::_basic_item* current_directory)
{
	//find
	if(0 == filename.compare(current_directory->get_filename()))
	{
		return current_directory;
	}

	if(S_ISDIR(current_directory->item_type()))
	{
		_basic_dir* directory_p=dynamic_cast<_basic_dir*>(current_directory);
		_basic_dir::items_set_t& files=directory_p->get_files();
		_basic_dir::_basic_item* ret=nullptr;
		for(_basic_dir::items_set_t::iterator it=files.begin(); it != files.end(); ++it)
		{
			if(nullptr != (ret=CBB::Common::find_by_name(filename, *it)))
			{
				return ret;
			}
		}
	}
	return nullptr;
}
