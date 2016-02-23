#include "CBB_serialization.h"
#include "CBB_const.h"

using namespace std;
using namespace CBB::Common;

fstream* CBB_serialization::file_open(const char* filename)
{
	if(file.is_open())
	{
		file.close();
	}
	file.open(filename, fstream::out | fstream::in);
	return &file;
}

fstream* CBB_serialization::file_open(const std::string& filename)
{
	return file_open(filename.c_str());
}

void CBB_serialization::file_close()
{
	if(file.is_open())
	{
		file.close();
	}
}
