#include "CBB_socket.h"

using namespace CBB::Common;
CBB_socket::CBB_socket():
	CBB_mutex_locker(),
	socket(-1)
{}

CBB_socket::CBB_socket(int socket):
	CBB_mutex_locker(),
	socket(socket)
{}

int CBB_socket::init(int socket)
{
	this->socket=socket;
}

int CBB_socket::get()
{
	return socket;
}
