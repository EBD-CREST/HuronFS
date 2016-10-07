#include "CBB_IO_task.h"

using namespace CBB::Common;
using namespace std;

basic_IO_task::basic_IO_task():
	basic_task(),
	connection_task(),
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

basic_IO_task::basic_IO_task(int id, basic_IO_task* next):
	basic_task(id, next),
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

send_buffer_element::send_buffer_element(const char* buffer, size_t size):
	buffer(buffer),
	size(size)
{}

extended_IO_task::extended_IO_task():
	basic_IO_task(),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr)
{}

extended_IO_task::extended_IO_task(int id, extended_IO_task* next):
	basic_IO_task(id, next),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr)
{}

extended_IO_task::~extended_IO_task()
{
	if(nullptr != receive_buffer)
	{
		delete[] receive_buffer;
	}
}

