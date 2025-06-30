#include <twilight_ppc.h>

#define TW_THREAD_FLAG_ACTIVE    1
#define TW_THREAD_FLAG_SLEEPING  2

static int _tw_thread_count = 0;
static int _tw_next_id = 0;
static int _tw_cur_thread = 0;
static int _tw_highest_schedule = 0;

struct _tw_schedule {
	unsigned timeReadyHigh;
	unsigned timeReadyLow;
	unsigned callback;
};
static struct _tw_thread_schedule_list[1024] = {0};

struct _tw_thread {
	unsigned flags;
	int id;
	void *entry;
	void *userData;
	TwCondition waitingCv;
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
	_tw_thread[0].flags = TW_THREAD_FLAG_ACTIVE;
}

int TW_GetThreadCount(void) {
	return _tw_thread_count;
}

int TW_StartThread(void *userData, void *(*entry)(void*)) {
	if (_tw_next_id < 2)
		_tw_next_id = 2;
	int id = _tw_next_id;

	struct _tw_thread *thread = (void*)0;
	for (int i = 0; i < TW_PPC_MAX_THREADS; i++) {
		struct _tw_thread *t = &_tw_threads[(id + i) % TW_PPC_MAX_THREADS];
		if ((t->flags & TW_THREAD_FLAG_ACTIVE) == 0) {
			thread = t;
			if (i >= _tw_thread_count)
				_tw_thread_count = i + 1;
			break;
		}
	}

	if (thread) {
		thread->flags = TW_THREAD_FLAG_ACTIVE;
		thread->id = id;
		thread->entry = entry;
		thread->userData = userData;
		unsigned *regs = (unsigned*)&thread->registers;
		for (int i = 0; i < sizeof(TW_PpcCpuContext) / 4; i++)
			regs[i] = 0;
	}

	return 0;
}

void TW_MaybeAutoSwitchContext(void) {
	// There could be a more sophisticated system here, eg. that allows for user-specified priority
	for (int i = 0; i < _tw_thread_count; i++) {
		if (i != _tw_cur_thread && (_tw_threads[i].flags & TW_THREAD_FLAG_ACTIVE)) {
			_tw_threads[_tw_cur_thread].flags |= TW_THREAD_FLAG_SLEEPING;
			TW_SwitchContext(&_tw_threads[_tw_cur_thread].registers, &_tw_threads[i].registers);
		}
	}
}

int TW_ScheduleTask(int durationUs, void (*callback)(void)) {
	if (durationUs <= 0) {
		callback();
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
		if ((s->callback & 1) == 0) {
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
	s->callback = (unsigned)callback | 1;
	s->timeReadyHigh = (unsigned)(readyTime >> 32);
	s->timeReadyLow = (unsigned)(readyTime & 0xffffFFFFull);

	if (shouldSchedule)
		TW_SetTimerInterrupt(_tw_dispatch_thread, deltaTime);

	return 0;
}

void TW_Sleep(int durationUs) {
	if (durationUs <= 0)
		return;

	TwMutex mtx = TW_CreateMutex();
	TwCondition cv = TW_CreateCondition();
	TW_LockMutex(mtx);

	TW_AwaitCondition(cv, mtx, durationUs);

	TW_UnlockMutex(mtx);
	TW_DestroyCondition(cv);
	TW_DestroyMutex(mtx);
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

	/*
	int deadlocked = 1;
	for (int i = 0; i < _tw_thread_count; i++) {
		if (i != _tw_cur_thread && (_tw_threads[i].flags & (TW_THREAD_FLAG_SLEEPING | TW_THREAD_FLAG_ACTIVE)) == TW_THREAD_FLAG_ACTIVE) {
			deadlocked = 0;
			break;
		}
	}
	if (deadlocked) {
		// we might be waiting for an interrupt, so this may not be a true deadlock
	}
	*/

	TW_EnableInterrupts(); // otherwise we'd definitely be blocked

	while (TW_GetAndSetAtomic((unsigned*)mutex, 1)) {
		TW_MaybeAutoSwitchContext();
	}

	_tw_threads[_tw_cur_thread].flags &= ~TW_THREAD_FLAG_SLEEPING;
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

// true if timed out
int TW_AwaitCondition(TwCondition cv, TwMutex mutex, int timeoutUs) {
	unsigned long long deltaTime = 243ull * (unsigned long long)durationUs;
	unsigned long long startTime = TW_GetCpuTimeBase();
	unsigned long long readyTime = startTime + deltaTime;

	TW_UnlockMutex(mutex);
	TW_OrAtomic((unsigned*)cv, 1);

	while ((PEEK_U32((unsigned)cv) & 1) == 1) {
		unsigned long long currentTime = TW_GetCpuTimeBase();
		if (currentTime >= readyTime) {
			_tw_threads[_tw_cur_thread].waitingCv = (void*)0;
			_tw_threads[_tw_cur_thread].flags &= ~TW_THREAD_FLAG_SLEEPING;
			TW_LockMutex(mutex);
			return 1;
		}

		_tw_threads[_tw_cur_thread].waitingCv = cv;
		if (timeoutUs > 0)
			TW_ScheduleTask(_tw_cur_thread, timeoutUs);
		TW_MaybeAutoSwitchContext();
		PPC_SYNC();
	}

	_tw_threads[_tw_cur_thread].waitingCv = (void*)0;
	_tw_threads[_tw_cur_thread].flags &= ~TW_THREAD_FLAG_SLEEPING;
	TW_LockMutex(mutex);
	return 0;
}

void TW_BroadcastCondition(TwCondition cv, TwMutex mutex) {
	TW_AndAtomic((unsigned*)cv, 0xFFFFfffe);
}

void TW_DestroyCondition(TwCondition cv) {
	TW_RemoveSb256Item(&_tw_threading_primitives, cv);
}
