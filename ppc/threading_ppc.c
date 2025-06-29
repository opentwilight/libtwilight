#include <twilight_ppc.h>

static int _tw_thread_count = 0;
static int _tw_next_id = 0;
static int _tw_cur_thread = 0;
static int _tw_highest_schedule = 0;

struct _tw_schedule {
	unsigned timeReadyHigh;
	unsigned timeReadyLow;
	int threadId;
};
static struct _tw_thread_schedule_list[1024] = {0};

struct _tw_thread {
	unsigned flags;
	int id;
	void *entry;
	void *userData;
	TW_PpcCpuContext registers;
	unsigned padding;
	// more info about the current thread
};
static struct _tw_thread _tw_threads[TW_PPC_MAX_THREADS] = {0};

// 32+1 256-bit (or 32 byte) blocks. +1 to account for padding.
char _tw_first_threading_prims_buf[1056];
TwSlabBucket256 _tw_threading_primitives = {};

void TW_SetupThreading(void) {
	_tw_threading_primitives = TW_CreateSlabBucket256(_tw_first_threading_prims_buf);
}

int TW_GetThreadCount(void) {
	return 1;
}

int TW_StartThread(void *userData, void *(*entry)(void*)) {
	TW_DisableInterrupts();

	if (_tw_next_id < 2)
		_tw_next_id = 2;
	int id = _tw_next_id;

	struct _tw_thread *thread = (void*)0;
	for (int i = 0; i < TW_PPC_MAX_THREADS; i++) {
		struct _tw_thread *t = &_tw_threads[(id + i) % TW_PPC_MAX_THREADS];
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
		unsigned *regs = (unsigned*)&thread->registers;
		for (int i = 0; i < sizeof(TW_PpcCpuContext) / 4; i++)
			regs[i] = 0;
	}

	TW_EnableInterrupts();
	return 0;
}

int TW_ScheduleTask(int threadId, int durationUs) {
	if (durationUs <= 0) {
		TW_SwitchContext(threadId);
		return 0;
	}

	unsigned long long deltaTime = 243ull * (unsigned long long)durationUs;
	if (deltaTime <= (1ull << 31)) {
		return -1;
	}

	unsigned long long currentTime = TW_GetCpuTimeBase();
	unsigned long long readyTime = currentTime + deltaTime;

	// TODO: Use a binary heap instead -- should speed up finding the nearest schedule item from O(n) to O(1)

	int slot = -1;
	int shouldSchedule = 0;
	for (int i = 0; i < _tw_highest_schedule && (slot < 0 || !shouldSchedule); i++) {
		struct _tw_schedule *s = &_tw_thread_schedule_list[i];
		if (s->threadId < 0) {
			slot = i;
			continue;
		}
		if (!shouldSchedule) {
			if (readyTime < (((unsigned long long)s->timeReadyHigh << 32) |
				((unsigned long long)s->timeReadyLow & 0xffffFFFFull))
			) {
				shouldSchedule = 1;
			}
		}
	}
	if (slot < 0)
		slot = _tw_highest_schedule & 0x3ff;

	struct _tw_schedule *s = &_tw_thread_schedule_list[i];
	s->threadId = threadId;
	s->timeReadyHigh = (unsigned)(readyTime >> 32);
	s->timeReadyLow = (unsigned)(readyTime & 0xffffFFFFull);

	if (shouldSchedule) {
		TW_SetTimerInterrupt(_tw_dispatch_thread, deltaTime);
	}

	return 0;
}

int TW_Sleep(int durationUs) {
	
}

TwMutex TW_CreateMutex(void) {
	void *ptr = TW_AddSb256Item(&_tw_threading_primitives);
	TW_ZeroAndFlushBlock(ptr);
	return ptr;
}

// TODO a lot of testing required here

void TW_LockMutex(TwMutex mutex) {
	unsigned prev = TW_GetAndSetAtomic((unsigned*)mutex, 1);
	if (prev == 0)
		return; // uncontended

	// TODO
	// add this thread to a list of sleeping threads
	// check if we're now deadlocked
	// maybe switch context so someone else gets a go

	TW_EnableInterrupts(); // otherwise we'd definitely be blocked

	while (TW_GetAndSetAtomic((unsigned*)mutex, 1));

	// TODO remove this thread from the list of sleeping threads
}

void TW_UnlockMutex(TwMutex mutex) {
	unsigned prev = TW_GetAndSetAtomic((unsigned*)mutex, 0);
}

void TW_DestroyMutex(TwMutex mutex) {
	TW_RemoveSb256Item(&_tw_threading_primitives, mutex);
}

TwCondition TW_CreateCondition(void) {
	void *ptr = TW_AddSb256Item(&_tw_threading_primitives);
	TW_ZeroAndFlushBlock(ptr);
	return ptr;
}

int TW_AwaitCondition(TwCondition cv, TwMutex mutex, int timeoutUs) {
	TW_UnlockMutex(mutex);
	TW_GetAndSetAtomic((unsigned*)cv, 1);
	TW_EnableInterrupts();
	// TODO maybe switch context here so someone else gets a go
	while ((PEEK_U32((unsigned)cv) & 1) == 0)
		PPC_SYNC();
	TW_LockMutex(mutex);
	// TODO return bool indicating whether we timed out
	return 0;
}

void TW_BroadcastCondition(TwCondition cv, TwMutex mutex) {
	TW_OrAtomic((unsigned*)cv, 1);
}

void TW_DestroyCondition(TwCondition cv) {
	TW_RemoveSb256Item(&_tw_threading_primitives, cv);
}
