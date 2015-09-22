#include "CBB_mutex_locker.h"

using namespace CBB::Common;

CBB_mutex_locker::CBB_mutex_locker():
#ifdef MUTLIPLE_THREAD
	locker(PTHREAD_MUTEX_INITIALIZER)
#endif
{}

CBB_mutex_locker::~CBB_mutex_locker()
{
#ifdef MUTLIPLE_THREAD
	pthread_mutex_destory(&locker);
#endif
}
