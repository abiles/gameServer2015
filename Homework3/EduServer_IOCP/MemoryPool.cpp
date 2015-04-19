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
	MemAllocInfo* mem = nullptr;
	
	//TODO: InterlockedPopEntrySList를 이용하여 mFreeList에서 pop으로 메모리를 가져올 수 있는지 확인. 

	// 싱글 링크드 리스트 앞쪽 한 item 지움, list로의 접근이 동기화 됨
	// return value는 제거된 item의 포인터, 비어 있으면 NULL을 return
	// return 값이 null이라는게 할당할 메모리가 없다는 뜻이 되겠지
	// listhead가 있으면 pop 이후에 null을 리턴하지 않는듯
	// head만 있는 상황에서는 head를 제거하지 않고 null을 리턴 하려나 

	mem = reinterpret_cast<MemAllocInfo*>(InterlockedPopEntrySList(&mFreeList));
	
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
	//무엇을 반납? => 메모리를 반납
	//아직 정확히 모르겠지만 가리키는 부분을 어떻게 해주고 push해야 할 것같은데
	

	InterlockedPushEntrySList(&mFreeList, ptr);

	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	// 일단 테이블에 아무것도 없게 초기화 
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	// memoryPool은 종류별로 만들어진 smallSizeMemorypool을 이용해서 
	// 만드는구나

	// 한번에 32씩 커지면서 smallPoll 생성해서 넣어주기
	for (int i = 32; i < 1024; i+=32)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		
		// 여기서 처음 한 사이클만 보면
		// 일단 j==1 부터 j==32까지
		// 위에서 만든 pool을 넣고 있다
		// pooltable에서 1~32까지는 같은것을 가리키고 있다는 것
		// header랑 사이즈만 초기화된 pool을
		// 관리하는 테이블의 1~32번가지 넣어주는 작업
		// 이후에는 33~64애들은 크기가 64인 smallpool 가리킴
		// 이 작업 반복... 등을 해준다
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
	for (int i = 2048; i < 4096; i += 256)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent + 1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}
	

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
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop();
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	// realSize일 때 맨 앞에 위치를 가리키는 header
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< 최근 할당에 관련된 정보 힌트
	
	// mAllocSize를 저장함
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< 두번 해제 체크 위해
	
	CRASH_ASSERT(realAllocSize> 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool에 (재사용을 위해) push..
		mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
		
	}
}