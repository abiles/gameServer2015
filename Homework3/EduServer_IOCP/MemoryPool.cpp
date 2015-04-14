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
	MemAllocInfo* mem = 0;
	
	//TODO: InterlockedPopEntrySList�� �̿��Ͽ� mFreeList���� pop���� �޸𸮸� ������ �� �ִ��� Ȯ��. 

	// �̱� ��ũ�� ����Ʈ ���� �� item ����, list���� ������ ����ȭ ��
	// return value�� ���ŵ� item�� ������, ��� ������ NULL�� return
	// ����Ʈ�� ���� �մ� �޸𸮸� ������ �ִ� �Ŷ��??
	// return ���� null�̶�°� �Ҵ��� �޸𸮰� ���ٴ� ���� �ǰ���
	// ���� list�� ��Ȯ�� �������� �ִ��� �𸣰���

	if (NULL == InterlockedPopEntrySList(&mFreeList));
	{
		// ��� ������? 
		
	}

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
		
		// 0������ ����� �ִµ�
		// ���⼭ ó�� �� ����Ŭ�� ����
		// �ϴ� j==1 ���� j==32����
		// ������ ���� pool�� �ְ� �ִ�
		// pooltable���� 1~32������ �������� ����Ű�� �ִٴ� ��
		// 
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
		//header = 
	}

	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< �ֱ� �Ҵ翡 ���õ� ���� ��Ʈ
	
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< �ι� ���� üũ ����
	
	CRASH_ASSERT(realAllocSize> 0);

	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//TODO: SmallSizeMemoryPool�� (������ ����) push..
		
	}
}