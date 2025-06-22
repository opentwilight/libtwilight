#include <twilight_ppc.h>

static int _tw_thread_count = 0;
static int _tw_next_id = 0;

struct _tw_thread {
	unsigned flags;
	int id;
	void *entry;
	void *userData;
	TW_PpcCpuContext registers;
	unsigned padding;
	// more info about the current thread
};
struct _tw_thread _tw_threads[TW_PPC_MAX_THREADS];

int TW_MultiThreadingEnabled(void) {
	return 0;
}

int TW_StartThread(void *userData, void *(*entry)(void*)) {
	TW_DisableInterrupts();

	if (_tw_next_id < 2)
		_tw_next_id = 2;
	int id = _tw_next_id;

	_tw_thread *thread = (void*)0;
	for (int i = 0; i < TW_PPC_MAX_THREADS; i++) {
		_tw_thread *t = &_tw_threads[(id + i) % TW_PPC_MAX_THREADS];
		if ((t->flags & 1) == 0) {
			thread = t;
			break;
		}
	}

	if (thread) {
		thread->flags = 1;
		thread->id = id;
		thread->entry = entry;
		thread->userData = userData;
		TW_FillWords(&thread->registers, 0, sizeof(TW_PpcCpuContext) / 4 + 1);
	}

	TW_EnableInterrupts();
}

void TW_LockMutex(TwMutex mutex) {
	
}

void TW_UnlockMutex(TwMutex mutex) {
	
}

void TW_AwaitCondition(TwCondition cond, int timeoutMs) {
	
}

void TW_BroadcastCondition(TwCondition cond) {
	
}
