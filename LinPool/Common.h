#pragma once
#include <algorithm>

namespace lin_MemoryPool
{
	// 对齐数和大小定义
	constexpr size_t ALIGNMENT = 8;
	constexpr size_t MAX_BYTES = 256 * 1024;
	constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

	// 内存块头部信息
	struct BlockHeader
	{
		size_t size;
		bool inUse;
		BlockHeader* next;
	};

	// 大小类管理
	class SizeClass
	{
	public:
		static size_t roundUp(size_t bytes)
		{
			// 通过加 ALIGNMENT-1 进位，再利用掩码截断低位
			return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
		}

		static size_t getIndex(size_t bytes)
		{
			// 确保bytes至少为ALIGNMENT的大小
			bytes = std::max(bytes, ALIGNMENT);
			return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
		}
	};
}
