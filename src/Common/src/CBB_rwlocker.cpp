#include "CBB_rwlocker.h"

using namespace CBB::Common;
#ifdef MUTIPLE_THREAD
CBB_rwlocker::CBB_rwlocker():
	locker(PTHREAD_RWLOCKER_INITIALIZER)
#else
CBB_rwlocker::CBB_rwlocker()
#endif
{}

CBB_rwlocker::~CBB_rwlocker()
{
#ifdef MUTIPLE_THREAD
	pthread_rwlock_destory(&locker);
#endif
}
