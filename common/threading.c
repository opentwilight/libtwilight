#include <twilight.h>

int TW_MultiThreadingEnabled(void) {
	return 0;
}

void TW_StartThread(void *userData, void *(*entry)(void*)) {
	
}

void TW_LockMutex(TwMutex mutex) {
	
}

void TW_UnlockMutex(TwMutex mutex) {
	
}

void TW_AwaitCondition(TwCondition cond, int timeoutMs) {
	
}

void TW_BroadcastCondition(TwCondition cond) {
	
}

void TW_AwaitFuture(TwFuture *future) {
	
}

void TW_CompleteFuture(TwFuture *future, void *value) {
	
}

void TW_FailFuture(TwFuture *future, int res) {
	
}

void TW_ReachFuture(TwFuture *future, void *value, int res) {
	
}
