#include "stdafx.h"
#include "CircularBuffer.h"
#include <assert.h>




void CircularBuffer::Remove(size_t len)
{
	size_t cnt = len ;
	
	/// Read�� ���������� A�� �ִٸ� A�������� ���� ����

	if ( mARegionSize > 0 )
	{
		size_t aRemove = (cnt > mARegionSize) ? mARegionSize : cnt ;
		mARegionSize -= aRemove ;
		mARegionPointer += aRemove ;
		cnt -= aRemove ;
	}

	// ������ �뷮�� �� ������� B���� ���� 
	if ( cnt > 0 && mBRegionSize > 0 )
	{
		size_t bRemove = (cnt > mBRegionSize) ? mBRegionSize : cnt ;
		mBRegionSize -= bRemove ;
		mBRegionPointer += bRemove ;
		cnt -= bRemove ;
	}

	/// A������ ������� B�� A�� ����ġ 
	if ( mARegionSize == 0 )
	{
		if ( mBRegionSize > 0 )
		{
			/// ������ ��� ���̱�
			if ( mBRegionPointer != mBuffer )
				memmove(mBuffer, mBRegionPointer, mBRegionSize) ;
	
			mARegionPointer = mBuffer ;
			mARegionSize = mBRegionSize ;
			mBRegionPointer = nullptr ;
			mBRegionSize = 0 ;
		}
		else
		{
			mBRegionPointer = nullptr ;
			mBRegionSize = 0 ;
			mARegionPointer = mBuffer ;
			mARegionSize = 0 ;
		}
	}
}


