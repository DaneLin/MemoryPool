#include "SimpleMemoryPool.h"

#include <cassert>

namespace Lin_SimpleMemoryPool
{
	MemoryPool::MemoryPool(size_t BlockSize)
		: BlockSize_(BlockSize)
		, SlotSize_(0)
		, firstBlock_(nullptr)
		, curSlot_(nullptr)
		, freeList_(nullptr)
		, lastSlot_(nullptr)
	{
	}

	MemoryPool::~MemoryPool()
	{
		// 把连续的block删除
		Slot* cur = firstBlock_;
		while (cur)
		{
			Slot* next = cur->next;
			// 等同于 free(reinterpret_cast<void*>(firstBlock_));
			// 转化为 void 指针，因为 void 类型不需要调用析构函数，只释放空间
			operator delete(reinterpret_cast<void*>(cur));
			cur = next;
		}
	}

	void MemoryPool::init(size_t size)
	{
		assert(size > 0);
		SlotSize_ = size;
		firstBlock_ = nullptr;
		curSlot_ = nullptr;
		freeList_ = nullptr;
		lastSlot_ = nullptr;
	}

	void* MemoryPool::allocate()
	{
		// 优先使用空闲链表中的内存槽
		Slot* slot = popFreeList();
		if (slot != nullptr)
		{
			// 当前内存块已经没有内存槽可以用，开辟一块新的内存
			allocateNewBlock();
		}

		Slot* temp;
		{
			std::lock_guard<std::mutex> lock(mutexForBlock_);

			if (curSlot_ >= lastSlot_)
			{
				allocateNewBlock();
			}

			temp = curSlot_;
			curSlot_ += SlotSize_ / sizeof(Slot);
		}

		return temp;
	}

	void MemoryPool::deallocate(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

		Slot* slot = reinterpret_cast<Slot*>(ptr);
		pushFreeList(slot);
	}

	void MemoryPool::allocateNewBlock()
	{
		// 头插法插入新的内存块
		void* newBlock = operator new(BlockSize_);
		reinterpret_cast<Slot*>(newBlock)->next = firstBlock_;
		firstBlock_ = reinterpret_cast<Slot*>(newBlock);

		char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);
		size_t paddingSize = padPointer(body, SlotSize_); // 计算对齐需要填充内存的大小
		curSlot_ = reinterpret_cast<Slot*>(body + paddingSize);

		// 超过该标记位置，则说明该内存块已经没有内存槽可以用，需要向系统申请新的内存块
		lastSlot_ = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);

		freeList_ = nullptr;
	}

	size_t MemoryPool::padPointer(char* p, size_t align)
	{
		// align 是槽的大小
		return (align - reinterpret_cast<size_t>(p)) % align;
	}


	bool MemoryPool::pushFreeList(Slot* slot)
	{
		while (true)
		{
			// 获取当前头节点
			Slot* oldHead = freeList_.load(std::memory_order_relaxed);
			// 将新节点的next指向当前头节点
			slot->next.store(oldHead, std::memory_order_relaxed);

			// CAS操作，将新节点设置为头节点
			if (freeList_.compare_exchange_weak(oldHead, slot, std::memory_order_release, std::memory_order_relaxed))
			{
				return true;
			}
		}
	}

	Slot* MemoryPool::popFreeList()
	{
		while (true)
		{
			Slot* oldHead = freeList_.load(std::memory_order_acquire);
			if (oldHead == nullptr)
			{
				return nullptr;
			}

			// 在访问 newHead 之前再次验证 oldHead 的有效性
			Slot* newHead = nullptr;
			try
			{
				newHead = oldHead->next.load(std::memory_order_relaxed);
			}
			catch (...)
			{
				// 如果返回失败，则continue重新尝试申请内存
				continue;
			}

			// 尝试更新头结点
			// 原子性地尝试将 freeList_ 从 oldHead 更新为 newHead
			if (freeList_.compare_exchange_weak(oldHead, newHead,
				std::memory_order_acquire, std::memory_order_relaxed))
			{
				return oldHead;
			}
			// 失败：说明另一个线程可能已经修改了 freeList_
			// CAS 失败则重试
		}
	}

	void HashBucket::initMemoryPool()
	{
		for (int i = 0; i < MEMORY_POOL_NUM; i++)
		{
			getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
		}
	}

	MemoryPool& HashBucket::getMemoryPool(int index)
	{
		static MemoryPool memoryPool[MEMORY_POOL_NUM];
		return memoryPool[index];
	}
}
