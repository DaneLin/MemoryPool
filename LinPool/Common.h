#pragma once
#include <algorithm>

namespace lin_MemoryPool
{
	// �������ʹ�С����
	constexpr size_t ALIGNMENT = 8;
	constexpr size_t MAX_BYTES = 256 * 1024;
	constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

	// �ڴ��ͷ����Ϣ
	struct BlockHeader
	{
		size_t size;
		bool inUse;
		BlockHeader* next;
	};

	// ��С�����
	class SizeClass
	{
	public:
		static size_t roundUp(size_t bytes)
		{
			// ͨ���� ALIGNMENT-1 ��λ������������ضϵ�λ
			return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
		}

		static size_t getIndex(size_t bytes)
		{
			// ȷ��bytes����ΪALIGNMENT�Ĵ�С
			bytes = std::max(bytes, ALIGNMENT);
			return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
		}
	};
}
