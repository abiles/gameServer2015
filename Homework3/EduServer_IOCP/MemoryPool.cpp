#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"

MemoryPool* GMemoryPool = nullptr;


SmallSizeMemoryPool::SmallSizeMemoryPool(DWORD allocSize) : mAllocSize(allocSize)
{
	CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
	
	// SLIST 헤더 초기화
	// 모든 리스트의 항목들은 MEMORY_ALLOCATION_ALIGNMENT으로 정렬되어 있어야하고
	// 정렬안되어 있으면 unpredictable result 발생
	InitializeSListHead(&mFreeList);
}

MemAllocInfo* SmallSizeMemoryPool::Pop()
{
	MemAllocInfo* mem = 0;
	
	//TODO: InterlockedPopEntrySList를 이용하여 mFreeList에서 pop으로 메모리를 가져올 수 있는지 확인. 

	// 싱글 링크드 리스트 앞쪽 한 item 지움, list로의 접근이 동기화 됨
	// return value는 제거된 item의 포인터, 비어 있으면 NULL을 return
	// 리스트가 남아 잇는 메모리를 가지고 있다면?
	// return 값이 null이라는게 할당할 메모리가 없다는 뜻이 되겠지
	if (NULL == InterlockedPopEntrySList(&mFreeList));
	{
		// 어떻게 해주지? 
		// 아직까지는 list에 뭐가 들어있는지 모르겠네
	}

	if (NULL == mem)
	{
		// 할당 불가능하면 직접 할당.
		mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		CRASH_ASSERT(mem->mAllocSize == 0);
	}

	InterlockedIncrement(&mAllocCount);
	return mem;
}

void SmallSizeMemoryPool::Push(MemAllocInfo* ptr)
{
	//TODO: InterlockedPushEntrySList를 이용하여 메모리풀에 (재사용을 위해) 반납.
	//무엇을 반납?

	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	for (int i = 32; i < 1024; i+=32)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent+1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	for (int i = 1024; i < 2048; i += 128)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent + 1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	//TODO: [2048, 4096] 범위 내에서 256바이트 단위로 SmallSizeMemoryPool을 할당하고 
	//TODO: mSmallSizeMemoryPoolTable에 O(1) access가 가능하도록 SmallSizeMemoryPool의 주소 기록

	

}

void* MemoryPool::Allocate(int size)
{
	MemAllocInfo* header = nullptr;
	int realAllocSize = size + sizeof(MemAllocInfo);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		header = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(realAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//TODO: SmallSizeMemoryPool에서 할당
		//header = ...; 
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< 최근 할당에 관련된 정보 힌트
	
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< 두번 해제 체크 위해
	
	CRASH_ASSERT(realAllocSize> 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool에 (재사용을 위해) push..
		
	}
}