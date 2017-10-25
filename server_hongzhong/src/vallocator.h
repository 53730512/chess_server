#pragma once

#ifndef _WIN32
#include <unistd.h>
#endif
#define thread_local _declspec(thread)

#define VSA_BRK  1
#define VSA_MMAP_MAP_PRIVATE 2
#define VSA_MMAP_MAP_MAP_ANONYMOUS 3

#define VSA_USE_SYSTEM_MEMFUNC VSA_MMAP_MAP_MAP_ANONYMOUS

#define VariableSizeAllocatorAllocBit_MIN (5)
#define VariableSizeAllocatorAllocBit_MAX (16)

#define VariableSizeAllocatorAlloc_PreAll

struct VariableSizeAllocator_Lock
{
	VariableSizeAllocator_Lock(std::atomic_flag& _abool)
	{
		abool = &_abool;
		while(abool->test_and_set());
	}
	VariableSizeAllocator_Lock(std::atomic_flag& _abool, bool lockit)
	{
		abool = &_abool;
		if( lockit )
			while(abool->test_and_set());
	}
	bool TryLock()
	{
		return !abool->test_and_set();
	}
	~VariableSizeAllocator_Lock()
	{
		Unlock();
	}
	void Unlock()
	{
		if( abool )
			abool->clear();
		abool = NULL;
	}

	std::atomic_flag* abool;
};

class VariableSizeAllocator_MemTool
{
public:
	static std::atomic_flag& LockCore()
	{
		static std::atomic_flag ab;
		return ab;
	}
	static void* Malloc(size_t size)
	{
		VariableSizeAllocator_Lock lock(LockCore());
#ifdef _WIN32
		return malloc(size);
#else
#if (VSA_USE_SYSTEM_MEMFUNC == VSA_BRK)
		void* ptr = sbrk(size);
		if( ptr == (void*)-1 )
			return NULL;
		return ptr;
#elif (VSA_USE_SYSTEM_MEMFUNC == VSA_MMAP_MAP_PRIVATE)
		static size_t pagesize = getpagesize();
		static int fd = open("/dev/zero", O_RDONLY);

		size_t needpage = size / pagesize;
		if (size % pagesize) needpage += 1;

		void* ptr = mmap(NULL, needpage * pagesize, PROT_WRITE, MAP_PRIVATE, fd, 0);
		return ptr;
#elif (VSA_USE_SYSTEM_MEMFUNC == VSA_MMAP_MAP_MAP_ANONYMOUS)
		static size_t pagesize = getpagesize();
		//static int fd = open("/dev/zero", O_RDONLY);

		size_t needpage = size / pagesize;
		if (size % pagesize) needpage += 1;

		void* ptr = mmap(NULL, needpage * pagesize, PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		return ptr;
#endif
#endif
	}

	static void Free(void* p, size_t size)
	{
		VariableSizeAllocator_Lock lock(LockCore());
#ifdef _WIN32
		return free(p);
#else
		munmap(p, size);
#endif
	}
};

struct VariableSizeAllocator_Ptr
{
	VariableSizeAllocator_Ptr* next;
	unsigned short protect_short;
	unsigned char threadid;
	char szindex;
	unsigned int protect_int;
	char data[];
};
#define VariableSizeAllocator_Ptr_SetProtectMark(ptr) do {\
	(ptr)->protect_short = 0x1E2E; \
	(ptr)->protect_int = 0x1F2F3F4F;\
} while(0)
#define VariableSizeAllocator_Ptr_CheckProtectMark(ptr) \
	((ptr)->protect_short == 0x1E2E && (ptr)->protect_int == 0x1F2F3F4F)
#define VariableSizeAllocator_Ptr_DestoryProtectMark(ptr) do {\
	(ptr)->protect_short = 0xFEFE;\
	(ptr)->protect_int = 0xFEFEFEFE;\
	*((unsigned long long*)((ptr)->data)) = 0xFEFEFEFEFEFEFEFE;\
} while(0)
#define VariableSizeAllocator_Ptr_IsDestoryProtectMark(ptr) \
	((ptr)->protect_short == 0xFEFE && (ptr)->protect_int == 0xFEFEFEFE)

class VariableSizeAllocator_PtrBox
{
public:
	VariableSizeAllocator_PtrBox()
	{
		clear();
	}

	~VariableSizeAllocator_PtrBox()
	{
		clear();
	}

	inline void clear()
	{
		count = 0;
		ptr_begin = NULL;
		ptr_end = NULL;
	}

	inline bool empty()
	{
		return (!ptr_begin);
	}

	inline void push_back(VariableSizeAllocator_Ptr* ptr)
	{
		if( empty() )
		{
			ptr_begin = ptr;
			ptr_end = ptr;
		}
		else
		{
			ptr_end->next = ptr;
			ptr_end = ptr;
		}

		ptr->next = NULL;
		count ++;
	}

	inline void push_back(VariableSizeAllocator_PtrBox& other)
	{
		if( other.empty() )
			return;

		if( empty() )
		{
			ptr_begin = other.ptr_begin;
			ptr_end = other.ptr_end;
			count = other.count;
		}
		else
		{
			ptr_end->next = other.ptr_begin;
			ptr_end = other.ptr_end;
			count += other.count;
		}
	}

	inline VariableSizeAllocator_Ptr* pop_front()
	{
		if( empty() )
		{
			return NULL;
		}
		else
		{
			VariableSizeAllocator_Ptr* rel = ptr_begin;
			ptr_begin = ptr_begin->next;
			//if( ptr_end == rel )
			//	ptr_end = NULL;
			count --;
			return rel;
		}
	}

	inline size_t size()
	{
		return count;
	}


private:
	size_t count;
	VariableSizeAllocator_Ptr* ptr_begin;
	VariableSizeAllocator_Ptr* ptr_end;
};

class VariableSizeAllocator_Pool
{
public:
	VariableSizeAllocator_Pool(unsigned char _tid, char _szindex, size_t _allocsize, size_t _premalloccount) 
	{
		threadid = _tid;
		allocsize = _allocsize;
		szindex = _szindex;
		premalloccount = _premalloccount;
		memset(&cb_free, 0, sizeof(cb_free));
		memset(&cb_back, 0, sizeof(cb_back));
		memset(&cb_otherback, 0, sizeof(cb_otherback));
		//
		PreMalloc();
	}

	~VariableSizeAllocator_Pool()
	{
		ClearFreed();
	}

	VariableSizeAllocator_Ptr* Malloc()
	{
		VariableSizeAllocator_Ptr* rel = cb_free.pop_front();
		if( !rel )
		{
			do{
				GetBack();
				if( rel = cb_free.pop_front() )
					break;
				//
				PreMalloc();
				if( rel = cb_free.pop_front() )
					break;
				//
				return NULL;
			} 
			while(0);
		}
		VariableSizeAllocator_Ptr_SetProtectMark(rel);
		rel->szindex = szindex;
		rel->threadid = threadid;
		return rel;
	}

	bool Free(VariableSizeAllocator_Ptr* rel, size_t& backout)
	{
		if( !VariableSizeAllocator_Ptr_CheckProtectMark(rel) )
			return false;
		if( rel->threadid == threadid )
		{
			cb_free.push_back(rel);
			backout = 0;
		}
		else
		{
			auto*& cb = cb_back[rel->threadid];
			if( cb == NULL )
				cb = new(VariableSizeAllocator_MemTool::Malloc(sizeof(VariableSizeAllocator_PtrBox))) VariableSizeAllocator_PtrBox();
			cb->push_back(rel);
			backout = cb->size();
		}
		return true;
	}

	void PreMalloc()
	{
		for(int j=0; j<32; j++)
		{
			char* p = (char*)VariableSizeAllocator_MemTool::Malloc(premalloccount * allocsize); //外层已经将开头的结构计算进内存分配了
			if( !p )
				return;
			for(size_t i=0; i<premalloccount; i++)
				cb_free.push_back((VariableSizeAllocator_Ptr*)(&p[i*allocsize]));
		}
	}

	void BackBlock(VariableSizeAllocator_Pool* sender_pool)
	{
		auto*& back = sender_pool->cb_back[threadid];
		auto& backto = cb_otherback[sender_pool->threadid];

		VariableSizeAllocator_PtrBox* zeroptr = NULL;
		if( !backto.compare_exchange_weak(zeroptr, back) ) //上次归还的还没用，不还
			return;

		//backto = back;
		back = NULL;
	}

	void GetBack()
	{
		for(int i=0; i<256; i++)
		{
			auto& ptrbox = cb_otherback[i];

			VariableSizeAllocator_PtrBox* workptr = ptrbox;
			if( !workptr )
				continue;
			if( !ptrbox.compare_exchange_weak(workptr, NULL) ) //上次归还的还没用，不还
				continue;
			//workptr = NULL;
			//
			cb_free.push_back(*workptr);
			//workptr->~VariableSizeAllocator_PtrBox();
			VariableSizeAllocator_MemTool::Free(workptr, sizeof(VariableSizeAllocator_PtrBox));
		}
	}

	void ClearFreed()
	{
		return;
		/*
		VariableSizeAllocator_Ptr* ptr;
		while(ptr = (VariableSizeAllocator_Ptr*)cb_free.pop_back())
			free(ptr);
		*/
	}

	size_t FreedCount()
	{
		return cb_free.size();
	}

	size_t get_premalloccount()
	{
		return premalloccount;
	}

	char get_szindex()
	{
		return szindex;
	}

private:
	unsigned char threadid;
	size_t allocsize;
	size_t premalloccount;
	char szindex;
	VariableSizeAllocator_PtrBox cb_free;
	VariableSizeAllocator_PtrBox* cb_back[256];
	std::atomic<VariableSizeAllocator_PtrBox*> cb_otherback[256];
};

class VariableSizeAllocator
{
public:
	VariableSizeAllocator(unsigned char _tid)
	{
		printf("VariableSizeAllocator: begin construct. \n");
		fflush(stdout);

		threadid = _tid;
		
		pools = new(VariableSizeAllocator_MemTool::Malloc(sizeof(VariableSizeAllocator_Pool*) * (VariableSizeAllocatorAllocBit_MAX+1))) VariableSizeAllocator_Pool*[VariableSizeAllocatorAllocBit_MAX+1]();
		for(size_t i=VariableSizeAllocatorAllocBit_MIN; i<=VariableSizeAllocatorAllocBit_MAX; i++)
			pools[i] = new(VariableSizeAllocator_MemTool::Malloc(sizeof(VariableSizeAllocator_Pool))) VariableSizeAllocator_Pool(threadid, (char)i, (size_t)1<<i, (size_t)1<<(VariableSizeAllocatorAllocBit_MAX-i+1));

		memset(malloctimes, 0, sizeof(malloctimes));
		memset(realloctimes, 0, sizeof(realloctimes));
		memset(freetimes, 0, sizeof(freetimes));
		memset(nowcount, 0, sizeof(nowcount));

		printf("VariableSizeAllocator: constructed. \n");
		fflush(stdout);

#ifdef VSA_USE_MALLOC_HOOK
#ifndef _WIN32
		if( _tid == 0 )
		{
			malloc_hook_user = malloc_hook_backup = __malloc_hook;
			realloc_hook_user = realloc_hook_backup = __realloc_hook;
			free_hook_user = free_hook_backup = __free_hook;
		}
#endif
#endif
	}

	~VariableSizeAllocator()
	{
#ifdef VSA_USE_MALLOC_HOOK
		SwitchMallocHook(false);
#endif

		printf("VariableSizeAllocator: begin disconstruct. \n");
		fflush(stdout);

		//ClearFreed();
		for(int i=VariableSizeAllocatorAllocBit_MIN; i<=VariableSizeAllocatorAllocBit_MAX; i++)
			delete pools[i];
		delete[] pools;

		printf("VariableSizeAllocator: disconstructed. \n");
		fflush(stdout);
	}

	void* Malloc(unsigned int size_want)
	{
		register unsigned int size = size_want + sizeof(VariableSizeAllocator_Ptr);

		if( size <= ((unsigned long long)1<<VariableSizeAllocatorAllocBit_MIN) )
		{
			//VariableSizeAllocator_Lock lock(isinused);
			malloctimes[VariableSizeAllocatorAllocBit_MIN]++;
			VariableSizeAllocator_Ptr* rel = pools[VariableSizeAllocatorAllocBit_MIN]->Malloc();
			if( !rel )
				return NULL;
			nowcount[VariableSizeAllocatorAllocBit_MIN]++;
			return rel->data;
		}
		else if( size > ((unsigned long long)1<<VariableSizeAllocatorAllocBit_MAX) )
		{
			int n = GetLengthBitBy64(size);

			//VariableSizeAllocator_Lock lock(isinused);
			malloctimes[n]++;
			VariableSizeAllocator_Ptr* ptr = (VariableSizeAllocator_Ptr*)VariableSizeAllocator_MemTool::Malloc((unsigned long long)1<<n);//malloc(size);
			if( !ptr )
				return NULL;
			nowcount[n]++;
			VariableSizeAllocator_Ptr_SetProtectMark(ptr);
			ptr->threadid = 0;
			ptr->szindex = (char)n;
			return ptr->data;
		}
		else
		{
			int n = GetLengthBitByMax(size);

			//VariableSizeAllocator_Lock lock(isinused);
			malloctimes[n] ++;
			VariableSizeAllocator_Ptr* rel = pools[n]->Malloc();
			if( !rel )
				return NULL;
			nowcount[n]++;
			return rel->data;
		}
	}

	void Free(void* ptr)
	{
		if( !ptr )
			return;

		register VariableSizeAllocator_Ptr* rel = (VariableSizeAllocator_Ptr*)((char*)ptr-sizeof(VariableSizeAllocator_Ptr));
		register char szindex = rel->szindex;

		if( szindex > VariableSizeAllocatorAllocBit_MAX )
		{
			if( !VariableSizeAllocator_Ptr_CheckProtectMark(rel) )
			{
__DATA_ERROR:
#ifdef VSA_USE_MALLOC_HOOK
				SwitchMallocHook(false);
#endif
				if( VariableSizeAllocator_Ptr_IsDestoryProtectMark(rel) )
				{
					printf("VariableSizeAllocator_Pool: fatal error!! data is broken down, maybe double free it.\n");
					fflush(stdout);
					assert("VariableSizeAllocator_Pool: fatal error!! data is broken down, maybe double free it.");
					throw;
					exit(948);
				}
				else
				{
					printf("VariableSizeAllocator_Pool: fatal error!! data is broken down.\n");
					fflush(stdout);
					assert("VariableSizeAllocator_Pool: fatal error!! data is broken down.");
					throw;
					exit(949);
				}
			}
			//VariableSizeAllocator_Lock lock(isinused);
			freetimes[szindex]++;
			//memset(rel, 0xFE, 1<<VariableSizeAllocatorAllocBit_MIN);  
			VariableSizeAllocator_Ptr_DestoryProtectMark(rel);
			//memset(rel->data, 0xFE, (1<<VariableSizeAllocatorAllocBit_MIN)-sizeof(VariableSizeAllocator_Ptr));
			VariableSizeAllocator_MemTool::Free(rel, (size_t)1<<szindex);//free(rel);
			nowcount[szindex]--;
		}
		else if( szindex < VariableSizeAllocatorAllocBit_MIN )
		{
			goto __DATA_ERROR;
		}
		else
		{
			//VariableSizeAllocator_Lock lock(isinused);
			freetimes[szindex]++;
			//SwitchMallocHook(false);
			size_t backout;
			if( !pools[szindex]->Free(rel, backout) )
			{
				goto __DATA_ERROR;
			}
			VariableSizeAllocator_Ptr_DestoryProtectMark(rel);
			//memset(rel->data, 0xFE, (1<<VariableSizeAllocatorAllocBit_MIN)-sizeof(VariableSizeAllocator_Ptr));
			if( backout > pools[szindex]->get_premalloccount()*32 )
			{
				VariableSizeAllocator* vsa = GetThreadAllocatorArray()[rel->threadid];
				vsa->BackBlock(pools[szindex]);
			}
			//SwitchMallocHook(true);
			nowcount[szindex]--;
		}
	}

	void* Realloc(void* ptr, unsigned int newsize)
	{
		size_t oldsz = GetPtrLength(ptr);
		void* newptr = Malloc(newsize);
		memcpy(newptr, ptr, (oldsz<newsize) ? oldsz : newsize);
		Free(ptr);

		int nold = GetLengthBitByMax(oldsz);
		int nnew = GetLengthBitByMax(newsize);

		//VariableSizeAllocator_Lock lock(isinused);
		freetimes[nold] --;
		malloctimes[nnew] --;
		realloctimes[nnew] ++;

		return newptr;
	}

	void BackBlock(VariableSizeAllocator_Pool* sender_pool)
	{
		pools[sender_pool->get_szindex()]->BackBlock(sender_pool);
	}

	size_t GetPtrLength(void* ptr)
	{
		if( !ptr )
			return 0;
		VariableSizeAllocator_Ptr* rel = (VariableSizeAllocator_Ptr*)((char*)ptr-sizeof(VariableSizeAllocator_Ptr));
		if( !VariableSizeAllocator_Ptr_CheckProtectMark(rel) )
		{
__DATA_ERROR:
#ifdef VSA_USE_MALLOC_HOOK
			SwitchMallocHook(false);
#endif
			if( VariableSizeAllocator_Ptr_IsDestoryProtectMark(rel) )
			{
				printf("VariableSizeAllocator_Pool: fatal error!! data is broken down, maybe double free it.\n");
				fflush(stdout);
				assert("VariableSizeAllocator_Pool: fatal error!! data is broken down, maybe double free it.");
				throw;
				exit(948);
			}
			else
			{
				printf("VariableSizeAllocator_Pool: fatal error!! data is broken down.\n");
				fflush(stdout);
				assert("VariableSizeAllocator_Pool: fatal error!! data is broken down.");
				throw;
				exit(949);
			}
		}
		return ((size_t)1 << rel->szindex) - sizeof(VariableSizeAllocator_Ptr);
	}

#ifdef VSA_USE_MALLOC_HOOK
	static void SwitchMallocHook(bool use_user)
	{
#ifndef _WIN32
		__malloc_hook = use_user ? malloc_hook_user : malloc_hook_backup;
		__realloc_hook = use_user ? realloc_hook_user : realloc_hook_backup;
		__free_hook = use_user ? free_hook_user : free_hook_backup;
#endif
	}

	static void SetMallocHook(
		void *(*in_malloc_hook)(size_t size, const void *caller),   
		void *(*in_realloc_hook)(void *ptr, size_t size, const void *caller),   
		void (*in_free_hook)(void *ptr, const void *caller)
		)
	{
		if( in_malloc_hook ) malloc_hook_user = in_malloc_hook;
		if( in_realloc_hook ) realloc_hook_user = in_realloc_hook;
		if( in_free_hook ) free_hook_user = in_free_hook;
	}
#endif

	void ClearFreed()
	{
		return;
		/*
		for(int i=VariableSizeAllocatorAllocBit_MIN; i<=VariableSizeAllocatorAllocBit_MAX; i++)
			pools[i]->ClearFreed();
		*/
	}

	void GetUsingTime(int bits, long long& malloctime, long long& realloctime, long long& freetime)
	{
		if( bits <= VariableSizeAllocatorAllocBit_MIN )
		{
			malloctime = malloctimes[VariableSizeAllocatorAllocBit_MIN];
			realloctime = realloctimes[VariableSizeAllocatorAllocBit_MIN];
			freetime = freetimes[VariableSizeAllocatorAllocBit_MIN];
		}
		else if( bits < 64 )
		{
			malloctime = malloctimes[bits];
			realloctime = realloctimes[bits];
			freetime = freetimes[bits];
		}
		else
		{
			malloctime = 0;
			realloctime = 0;
			freetime = 0;
		}
	}

	long long GetNowCount(int bits)
	{
		if( bits <= VariableSizeAllocatorAllocBit_MIN )
		{
			return nowcount[VariableSizeAllocatorAllocBit_MIN];
		}
		else if( bits < 64 )
		{
			return nowcount[bits];
		}
		else
		{
			return 0;
		}
	}

	long long GetFreedCount(int bits)
	{
		if( bits < VariableSizeAllocatorAllocBit_MIN )
			return pools[VariableSizeAllocatorAllocBit_MIN]->FreedCount();
		else if( bits <= VariableSizeAllocatorAllocBit_MAX )
			return pools[bits]->FreedCount();
		return 0;
	}

	void GetInfo(char* buffer, int sz)
	{
		char* p = buffer;
		for(int i=VariableSizeAllocatorAllocBit_MIN; i<64; i++)
		{
			long long alloccount, realloccount, freecount;
			GetUsingTime(i, alloccount, realloccount, freecount);
			long long sizenow = GetNowCount(i), freed = GetFreedCount(i);
			if( alloccount == 0 && freecount == 0 && sizenow == 0 && freed == 0 )
				continue;
			char szbuf[1024];
			if( i < 10 )
				_snprintf(szbuf, sizeof(szbuf), "%dB", (1<<i));
			else if( i < 20 )
				_snprintf(szbuf, sizeof(szbuf), "%dKB", (1<<(i-10)));
			else if( i < 30 )
				_snprintf(szbuf, sizeof(szbuf), "%dMB", (1<<(i-20)));
			else if( i < 40 )
				_snprintf(szbuf, sizeof(szbuf), "%dGB", (1<<(i-30)));
			else
				_snprintf(szbuf, sizeof(szbuf), "%dTB", (1<<(i-40)));

			p += _snprintf(p, buffer+sz-p, "  SIZE<=%s\t- USE:%lld(A:%lld,R:%lld,F:%lld) INUSED:%lld FREED:%lld\n", 
				szbuf,
				alloccount + realloccount + freecount, alloccount, realloccount, freecount, sizenow, freed);
		}
	}

	void ResetAllocAndFreeTimes()
	{
		//VariableSizeAllocator_Lock lock(isinused);

		memset(malloctimes, 0, sizeof(malloctimes));
		memset(realloctimes, 0, sizeof(realloctimes));
		memset(freetimes, 0, sizeof(freetimes));
	}

public:
	static VariableSizeAllocator* GetInstance()
	{
		static thread_local VariableSizeAllocator* vsa = NULL;
		if( vsa == NULL )
		{
			static std::atomic_flag isincreate;
			VariableSizeAllocator_Lock lock(isincreate);
			if( vsa == NULL )
			{
				VariableSizeAllocator** tarray = GetThreadAllocatorArray();
				int idx = 0;
				for(; idx<256; idx++)
				{
					if( !tarray[idx] )
						break;
				}
				if( idx == 256 ) //分配失败
					return NULL;
				vsa = new(VariableSizeAllocator_MemTool::Malloc(sizeof(VariableSizeAllocator))) VariableSizeAllocator(idx);
				tarray[idx] = vsa;
			}
		}
		return vsa;
	}

private:
	static VariableSizeAllocator** GetThreadAllocatorArray()
	{
		static VariableSizeAllocator* savearray[256] = {0};
		return savearray;
	}

	static int bsr_64(unsigned long long num)
	{
	#ifdef _WIN32
		unsigned long index;
		if( _BitScanReverse64(&index, num) )
			return 64 - 1 - index;
		return 64;
	#else
		if( num == 0 )
			return 64;
		return __builtin_clzll(num);
	#endif
	}

	inline static int GetLengthBitByMax(unsigned int size)
	{
		int n = VariableSizeAllocatorAllocBit_MAX - bsr_64((unsigned long long)size << ((sizeof(unsigned int)*8) - VariableSizeAllocatorAllocBit_MAX)) + 32;
		if( ((unsigned long long)size << 1) == ((unsigned long long)1 << n) )
			n --;
		return n;
	}

	inline static int GetLengthBitBy64(unsigned int size)
	{
		int n = 64 - bsr_64(size);
		if( ((unsigned long long)size << 1) == ((unsigned long long)1 << n) )
			n --;
		return n;
	}

	unsigned char threadid;
	VariableSizeAllocator_Pool** pools;
	std::atomic_flag isinused;
	long long malloctimes[64];
	long long realloctimes[64];
	long long freetimes[64];
	long long nowcount[64];
#ifdef VSA_USE_MALLOC_HOOK
	static void *(*malloc_hook_backup)(size_t size, const void *caller);   
	static void *(*realloc_hook_backup)(void *ptr, size_t size, const void *caller);   
	static void (*free_hook_backup)(void *ptr, const void *caller); 
	//
	static void *(*malloc_hook_user)(size_t size, const void *caller);   
	static void *(*realloc_hook_user)(void *ptr, size_t size, const void *caller);   
	static void (*free_hook_user)(void *ptr, const void *caller); 
#endif
};
