#include <twilight_ppc.h>

#define TW_THREAD_FLAG_ACTIVE   1
#define TW_THREAD_FLAG_RUNNING  2
#define TW_THREAD_FLAG_WAITING  4

extern void TW_SwitchContext(TwPpcCpuContext *fromCtx, TwPpcCpuContext *toCtx);
extern void TW_EnterContext(TwPpcCpuContext *ctx);

static int _tw_thread_count = 0;
static int _tw_cur_thread = 0;
static int _tw_highest_schedule = 0;

struct _tw_schedule {
	unsigned entryPoint;
	unsigned userData;
	unsigned timeReadyHigh;
	unsigned timeReadyLow;
};
static struct _tw_schedule _tw_thread_schedule_list[1024] = {0};

struct _tw_thread {
	unsigned flags;
	int id;
	void *stackStart;
	void *userData;
	TwCondition waitingCv;
	TwPpcCpuContext *registers;
};
static struct _tw_thread _tw_threads[TW_PPC_MAX_THREADS] = {0};

static TwPpcCpuContext _tw_thread0_regs = {0};

// 32+1 256-bit (or 32 byte) blocks. +1 to account for padding.
static char _tw_first_threading_prims_buf[1056];
static TwSlabBucket256 _tw_threading_primitives = {};

static TwMutex _tw_threading_prims_mtx = (void*)0;

void TW_EndThisThread(void);

void TW_SetupThreading(void) {
	if (_tw_thread_count > 0)
		return;

	// _tw_threading_primitives must be initialised first, since that's where all concurrency objects are allocated from
	_tw_threading_primitives = TW_CreateSlabBucket256(_tw_first_threading_prims_buf);
	_tw_threading_prims_mtx = TW_AddSb256Item(&_tw_threading_primitives);
	TW_ZeroAndFlushBlock((unsigned*)_tw_threading_prims_mtx);

	void *toc = TW_GetTocPointer();
	void *stack = TW_GetMainStackStart();

	_tw_threads[0].id = 0;
	_tw_threads[0].stackStart = stack;
	_tw_threads[0].userData = (void*)0;
	_tw_threads[0].waitingCv = (void*)0;
	_tw_threads[0].registers = &_tw_thread0_regs;

	_tw_thread0_regs.gprs[1] = (unsigned)stack;
	_tw_thread0_regs.gprs[2] = (unsigned)toc;
	_tw_thread0_regs.gprs[13] = (unsigned)toc;
	_tw_thread0_regs.lr = (unsigned)TW_Exit;

	_tw_threads[0].flags = TW_THREAD_FLAG_RUNNING | TW_THREAD_FLAG_ACTIVE;
	_tw_thread_count = 1;
}

int TW_StartThread(void *userData, void *(*entryPoint)(void*)) {
	void *stackStart = (void*)0;
	TwPpcCpuContext *registers = (void*)0;

	TW_DisableInterrupts();

	struct _tw_thread *thread = (void*)0;
	for (int i = 1; i < TW_PPC_MAX_THREADS; i++) {
		struct _tw_thread *t = &_tw_threads[i];
		if ((t->flags & TW_THREAD_FLAG_ACTIVE) == 0) {
			thread = t;
			thread->id = i;
			thread->flags |= TW_THREAD_FLAG_ACTIVE;
			stackStart = thread->stackStart;
			registers = thread->registers;
			if (i >= _tw_thread_count)
				_tw_thread_count = i + 1;
			break;
		}
	}

	TW_EnableInterrupts();

	if (!thread)
		return 0;

	if (!stackStart)
		stackStart = TW_AllocateHeapObject(TW_GetGlobalAllocator(), 0x8000, 4);
	if (!registers)
		registers = TW_AllocateHeapObject(TW_GetGlobalAllocator(), sizeof(TwPpcCpuContext), 1);

	void *toc = TW_GetTocPointer();

	TW_DisableInterrupts();

	thread->registers = registers;
	thread->registers->gprs[1] = (unsigned)stackStart;
	thread->registers->gprs[2] = (unsigned)toc;
	thread->registers->gprs[3] = (unsigned)userData;
	thread->registers->gprs[13] = (unsigned)toc;
	thread->registers->pc = (unsigned)entryPoint;
	thread->registers->lr = (unsigned)TW_EndThisThread;

	thread->stackStart = stackStart;
	thread->userData = userData;

	thread->flags |= TW_THREAD_FLAG_RUNNING;

	TW_EnableInterrupts();

	return 1;
}

static void _tw_dispatch_handler(void) {
	unsigned long long currentTime = TW_GetCpuTimeBase();
	unsigned long long lowestTime = -1ULL;
	void *userData = (void*)0;
	unsigned taskEntry = 0;

	TW_DisableInterrupts();

	// TODO: Use a binary heap instead -- should speed up finding the nearest schedule item from O(n) to O(1)

	for (int i = 0; i < _tw_highest_schedule; i++) {
		struct _tw_schedule *s = &_tw_thread_schedule_list[i];
		if ((s->entryPoint & 1) == 0) {
			continue;
		}
		unsigned long long t = ((unsigned long long)s->timeReadyHigh << 32) | ((unsigned long long)s->timeReadyLow & 0xffffFFFFull);
		if (t && t < lowestTime) {
			lowestTime = t;
			userData = (void*)s->userData;
			taskEntry = s->entryPoint & ~1;
		}
	}

	TW_EnableInterrupts();

	if (taskEntry) {
		void (*f)(void*) = (void (*)(void*))taskEntry;
		f(userData);
	}
}

static void _tw_join_thread_from_dispatch(void *args) {
	int threadId = (int)args;
	_tw_cur_thread = threadId;
	TW_EnterContext(_tw_threads[threadId].registers);
}

static void _maybe_auto_enter_context() {
	TW_DisableInterrupts();

	// TODO: There should be a more sophisticated system here, eg. that allows for user-specified priority
	// Another weakness of this design is that it starves threads that are further down the list
	for (int i = 0; i < _tw_thread_count; i++) {
		if (i != _tw_cur_thread && (_tw_threads[i].flags & TW_THREAD_FLAG_RUNNING)) {
			_tw_threads[_tw_cur_thread].flags = 0;
			_tw_threads[_tw_cur_thread].waitingCv = (void*)0;
			_tw_cur_thread = i;

			TW_EnableInterrupts();
			TW_EnterContext(_tw_threads[i].registers);
			TW_DisableInterrupts();
		}
	}

	TW_EnableInterrupts();
}

void TW_MaybeAutoSwitchContext(void) {
	TW_DisableInterrupts();

	// TODO: There should be a more sophisticated system here, eg. that allows for user-specified priority
	// Another weakness of this design is that it starves threads that are further down the list
	for (int i = 0; i < _tw_thread_count; i++) {
		if (i != _tw_cur_thread && (_tw_threads[i].flags & TW_THREAD_FLAG_RUNNING)) {
			_tw_cur_thread = i;

			TW_EnableInterrupts();
			TW_SwitchContext(_tw_threads[_tw_cur_thread].registers, _tw_threads[i].registers);
			TW_DisableInterrupts();
		}
	}

	TW_EnableInterrupts();
}

void TW_EndThisThread(void) {
	while (1) {
		_maybe_auto_enter_context();
		// We should have entered another context by now.
		// If we reach here, then we haven't, so try to see if there's a task that's ready by calling the dispatch handler directly.
		_tw_dispatch_handler();
		// If not, try again :)
	}
}

int TW_ScheduleTask(int durationUs, void *userData, void (*taskEntry)(void*)) {
	if (durationUs <= 0) {
		taskEntry(userData);
		return 0;
	}

	unsigned long long deltaTime = 243ull * (unsigned long long)durationUs;
	if (deltaTime <= (1ull << 31)) {
		return -1;
	}

	unsigned long long currentTime = TW_GetCpuTimeBase();
	unsigned long long readyTime = currentTime + deltaTime;

	TW_DisableInterrupts();

	// TODO: Use a binary heap instead -- should speed up finding the nearest schedule item from O(n) to O(1)

	int slot = -1;
	int shouldSchedule = 0;
	for (int i = 0; i < _tw_highest_schedule && (slot < 0 || !shouldSchedule); i++) {
		struct _tw_schedule *s = &_tw_thread_schedule_list[i];
		if ((s->entryPoint & 1) == 0) {
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

	struct _tw_schedule *s = &_tw_thread_schedule_list[slot];
	s->entryPoint = (unsigned)taskEntry | 1;
	s->userData = (unsigned)userData;
	s->timeReadyHigh = (unsigned)(readyTime >> 32);
	s->timeReadyLow = (unsigned)(readyTime & 0xffffFFFFull);

	if (shouldSchedule)
		TW_SetTimerInterrupt(_tw_dispatch_handler, deltaTime);

	TW_EnableInterrupts();

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
	TW_LockMutex(_tw_threading_prims_mtx);
	void *ptr = TW_AddSb256Item(&_tw_threading_primitives);
	TW_UnlockMutex(_tw_threading_prims_mtx);
	TW_ZeroAndFlushBlock(ptr);
	return ptr;
}

// TODO a lot of testing required here

void TW_LockMutex(TwMutex mutex) {
	unsigned prev = TW_GetAndSetAtomic((unsigned*)mutex, 1);
	if (prev == 0)
		return; // uncontended

	_tw_threads[_tw_cur_thread].flags |= TW_THREAD_FLAG_WAITING;
	PPC_SYNC();

	/*
	int deadlocked = 1;
	for (int i = 0; i < _tw_thread_count; i++) {
		if (i != _tw_cur_thread && (_tw_threads[i].flags & (TW_THREAD_FLAG_WAITING | TW_THREAD_FLAG_RUNNING)) == TW_THREAD_FLAG_RUNNING) {
			deadlocked = 0;
			break;
		}
	}
	if (deadlocked) {
		// we might be waiting for an interrupt, so this may not be a true deadlock
	}
	*/

	while (TW_GetAndSetAtomic((unsigned*)mutex, 1)) {
		TW_MaybeAutoSwitchContext();
	}

	_tw_threads[_tw_cur_thread].flags &= ~TW_THREAD_FLAG_WAITING;
	PPC_SYNC();
}

void TW_UnlockMutex(TwMutex mutex) {
	unsigned prev = TW_GetAndSetAtomic((unsigned*)mutex, 0);
}

void TW_DestroyMutex(TwMutex mutex) {
	TW_LockMutex(_tw_threading_prims_mtx);
	TW_RemoveSb256Item(&_tw_threading_primitives, mutex);
	TW_UnlockMutex(_tw_threading_prims_mtx);
}

TwCondition TW_CreateCondition(void) {
	TW_LockMutex(_tw_threading_prims_mtx);
	void *ptr = TW_AddSb256Item(&_tw_threading_primitives);
	TW_UnlockMutex(_tw_threading_prims_mtx);
	TW_ZeroAndFlushBlock(ptr);
	return ptr;
}

// true if timed out
int TW_AwaitCondition(TwCondition cv, TwMutex mutex, int durationUs) {
	unsigned long long deltaTime = 243ull * (unsigned long long)durationUs;
	unsigned long long startTime = TW_GetCpuTimeBase();
	unsigned long long readyTime = startTime + deltaTime;

	TW_UnlockMutex(mutex);
	TW_OrAtomic((unsigned*)cv, 1);

	while ((PEEK_U32((unsigned)cv) & 1) == 1) {
		unsigned long long currentTime = TW_GetCpuTimeBase();
		if (currentTime >= readyTime) {
			TW_LockMutex(mutex);
			return 1;
		}

		_tw_threads[_tw_cur_thread].waitingCv = cv;
		_tw_threads[_tw_cur_thread].flags |= TW_THREAD_FLAG_WAITING;
		PPC_SYNC();

		if (durationUs > 0)
			TW_ScheduleTask(durationUs, (void*)_tw_cur_thread, _tw_join_thread_from_dispatch);

		TW_MaybeAutoSwitchContext();

		_tw_threads[_tw_cur_thread].waitingCv = (void*)0;
		_tw_threads[_tw_cur_thread].flags &= ~TW_THREAD_FLAG_WAITING;
		PPC_SYNC();
	}

	TW_LockMutex(mutex);
	return 0;
}

void TW_BroadcastCondition(TwCondition cv, TwMutex mutex) {
	TW_AndAtomic((unsigned*)cv, 0xFFFFfffe);
}

void TW_DestroyCondition(TwCondition cv) {
	TW_LockMutex(_tw_threading_prims_mtx);
	TW_RemoveSb256Item(&_tw_threading_primitives, cv);
	TW_UnlockMutex(_tw_threading_prims_mtx);
}
