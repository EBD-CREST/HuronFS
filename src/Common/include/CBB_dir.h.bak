#ifndef CBB_FILE_ITEM_H_
#define CBB_FILE_ITEM_H_

#include <vector>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <set>

#include "CBB_rwlocker.h"
#include "CBB_map.h"
#include <string>

namespace CBB
{
	namespace Common
	{

		class _basic_item;
		typedef CBB_map<std::string, _basic_item*> file_full_path_t;
		extern _basic_item* find_by_name(const std::string filename, _basic_item* current_directory);

		class _basic_item: public CBB_rwlocker
		{
			public:
				friend _basic_item* find_by_name(const std::string filename, _basic_item* current_directory);
				_basic_item();
				_basic_item(const struct stat& files_status, const std::string& filename);
				virtual ~_basic_item();
				struct stat& get_status();
				virtual int item_type()const;
				const struct stat& get_status()const;
				std::string& get_filename();
				const std::string& get_filename()const;
				const std::string& get_filefullpath()const;
			private:
				struct stat status;
				std::string name;
				file_full_path_t::iterator full_path;
		};

		typedef _basic_item _basic_file;

		class _basic_dir
		{
			public:
				typedef std::set<_basic_item*> items_set_t;

				_basic_dir();
				virtual ~_basic_dir();
				items_set_t::iterator add_item(_basic_item* item);
				items_set_t& get_files();
				const items_set_t& get_files()const;
				bool remove_item(_basic_item* item);
				bool remove_item(const std::string& file_name);
				bool remove_item(const items_set_t::iterator& it);

			private:
				items_set_t items;
		};
		typedef _basic_dir CBB_dir;

		inline struct stat& _basic_item::get_status()
		{
			return status;
		}

		inline const struct stat& _basic_item::get_status()const
		{
			return status;
		}

		inline std::string& _basic_item::get_filename()
		{
			return name;
		}

		inline const std::string& _basic_item::get_filename()const
		{
			return name;
		}

		inline const std::string& _basic_item::get_filefullpath()const
		{
			return full_path->first;
		}

	}
}
#endif
