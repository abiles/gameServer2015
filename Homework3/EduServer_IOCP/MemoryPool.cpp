#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"

MemoryPool* GMemoryPool = nullptr;


SmallSizeMemoryPool::SmallSizeMemoryPool(DWORD allocSize) : mAllocSize(allocSize)
{
	CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
	
	// SLIST ��� �ʱ�ȭ
	// ��� ����Ʈ�� �׸���� MEMORY_ALLOCATION_ALIGNMENT���� ���ĵǾ� �־���ϰ�
	// ���ľȵǾ� ������ unpredictable result �߻�
	InitializeSListHead(&mFreeList);
}

MemAllocInfo* SmallSizeMemoryPool::Pop()
{
	MemAllocInfo* mem = nullptr;
	
	//TODO: InterlockedPopEntrySList�� �̿��Ͽ� mFreeList���� pop���� �޸𸮸� ������ �� �ִ��� Ȯ��. 

	// �̱� ��ũ�� ����Ʈ ���� �� item ����, list���� ������ ����ȭ ��
	// return value�� ���ŵ� item�� ������, ��� ������ NULL�� return
	// return ���� null�̶�°� �Ҵ��� �޸𸮰� ���ٴ� ���� �ǰ���
	// listhead�� ������ pop ���Ŀ� null�� �������� �ʴµ�
	// head�� �ִ� ��Ȳ������ head�� �������� �ʰ� null�� ���� �Ϸ��� 

	mem = reinterpret_cast<MemAllocInfo*>(InterlockedPopEntrySList(&mFreeList));
	
	if (NULL == mem)
	{
		// �Ҵ� �Ұ����ϸ� ���� �Ҵ�.
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
	//TODO: InterlockedPushEntrySList�� �̿��Ͽ� �޸�Ǯ�� (������ ����) �ݳ�.
	//������ �ݳ�? => �޸𸮸� �ݳ�
	//���� ��Ȯ�� �𸣰����� ����Ű�� �κ��� ��� ���ְ� push�ؾ� �� �Ͱ�����
	

	InterlockedPushEntrySList(&mFreeList, ptr);

	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	// �ϴ� ���̺� �ƹ��͵� ���� �ʱ�ȭ 
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	// memoryPool�� �������� ������� smallSizeMemorypool�� �̿��ؼ� 
	// ����±���

	// �ѹ��� 32�� Ŀ���鼭 smallPoll �����ؼ� �־��ֱ�
	for (int i = 32; i < 1024; i+=32)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		
		// ���⼭ ó�� �� ����Ŭ�� ����
		// �ϴ� j==1 ���� j==32����
		// ������ ���� pool�� �ְ� �ִ�
		// pooltable���� 1~32������ �������� ����Ű�� �ִٴ� ��
		// header�� ����� �ʱ�ȭ�� pool��
		// �����ϴ� ���̺��� 1~32������ �־��ִ� �۾�
		// ���Ŀ��� 33~64�ֵ��� ũ�Ⱑ 64�� smallpool ����Ŵ
		// �� �۾� �ݺ�... ���� ���ش�
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

	//TODO: [2048, 4096] ���� ������ 256����Ʈ ������ SmallSizeMemoryPool�� �Ҵ��ϰ� 
	//TODO: mSmallSizeMemoryPoolTable�� O(1) access�� �����ϵ��� SmallSizeMemoryPool�� �ּ� ���
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
		//TODO: SmallSizeMemoryPool���� �Ҵ�
		//header = ...;
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop();
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	// realSize�� �� �� �տ� ��ġ�� ����Ű�� header
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< �ֱ� �Ҵ翡 ���õ� ���� ��Ʈ
	
	// mAllocSize�� ������
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< �ι� ���� üũ ����
	
	CRASH_ASSERT(realAllocSize> 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool�� (������ ����) push..
		mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
		
	}
}