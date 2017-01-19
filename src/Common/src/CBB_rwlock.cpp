#include "CBB_rwlock.h"

using namespace CBB::Common;
#ifdef MUTIPLE_THREAD
CBB_rwlock::CBB_rwlock():
	lock(PTHREAD_RWLOCKER_INITIALIZER)
#else
CBB_rwlock::CBB_rwlock()
#endif
{}

CBB_rwlock::~CBB_rwlock()
{
#ifdef MUTIPLE_THREAD
	pthread_rwlock_destory(&lock);
#endif
}
