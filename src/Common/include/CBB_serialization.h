#ifndef CBB_SERIALIZATION_H_
#define CBB_SERIALIZATION_H_

#include <fstream>

namespace CBB
{
	namespace Common
	{
		class CBB_serialization
		{
			public:
				CBB_serialization()=default;
				virtual ~CBB_serialization()=default;
				template<typename type>int serialize(const type& obj, const std::string& filename);
				template<typename type>int deserialize(type& obj, const std::string& filename);
				template<typename type>int serialize(const type& obj, const char* filename);
				template<typename type>int deserialize(type& obj, const char* filename);
			private:
				std::fstream* file_open(const char* filename);
				std::fstream* file_open(const std::string& filename);
				void file_close();

			private:
				std::fstream file;
		};

#ifdef BOOST_SERIALIZATION

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>

		template<typename type>int CBB_serialization::serialize(const type& obj, const std::string& filename)
		{
			return CBB_serialization::serialize(obj, filename.c_str());
		}

		template<typename type>int CBB_serialization::serialize(const type& obj, const char* filename)
		{
			std::boost::archive::text_oarchive oarch(file_open(filename));
			oarch<<obj;
			file_close();
			return SUCCESS;
		}

		template<typename type>int CBB_serialization::deserialize(type& obj, const char* filename)
		{
			std::boost::archive::text_iarchive iarch(file_open(filename));
			iarch<<obj;
			file_close();
			return SUCCESS;
		}

		template<typename type>int CBB_serialization::deserialize(type& obj, const std::string& filename)
		{
			return CBB_serialization::deserialize(obj, filename.c_str());
		}

		//end of BOOST_SERIALIZATION
#endif
	}
}

#endif
