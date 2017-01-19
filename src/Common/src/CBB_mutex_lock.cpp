#include "CBB_mutex_lock.h"

using namespace CBB::Common;

CBB_mutex_lock::CBB_mutex_lock()
#ifdef MUTLIPLE_THREAD
	:lock(PTHREAD_MUTEX_INITIALIZER)
#endif
{}

CBB_mutex_lock::~CBB_mutex_lock()
{
#ifdef MUTLIPLE_THREAD
	pthread_mutex_destory(&lock);
#endif
}
