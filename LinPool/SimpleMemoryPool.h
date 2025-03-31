#pragma once

#include <atomic>
#include <mutex>

namespace Lin_SimpleMemoryPool
{
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512

	struct Slot
	{
		std::atomic<Slot*> next; 
	};

	class MemoryPool
	{
	public:
		MemoryPool(size_t BlockSize = 4096);
		~MemoryPool();

		void init(size_t);

		void* allocate();
		void deallocate(void*);

	private:
		void allocateNewBlock();
		size_t padPointer(char* p, size_t align);

		// CAS进行无锁入队和出队
		bool pushFreeList(Slot* slot);
		Slot* popFreeList();

	private:
		int BlockSize_; // 内存块大小
		int SlotSize_; // 每个Slot的大小
		Slot* firstBlock_; // 指向内存池管理的首个实际内存块
		Slot* curSlot_; // 指向当前未被使用过的槽
		std::atomic<Slot*> freeList_; // 指向空闲槽链表的头部
		Slot* lastSlot_; // 作为当前内存块中最后能够存放元素的位置标识(超过该位置需申请新的内存块)
		std::mutex mutexForBlock_; // 保证多线程情况下避免不必要的重复开辟内存导致的浪费行为。

	};

	class HashBucket
	{
	public:
		static void initMemoryPool();
		static MemoryPool& getMemoryPool(int index);

		static void* useMemory(size_t size)
		{
			if (size <= 0)
			{
				return nullptr;
			}
			if (size > MAX_SLOT_SIZE)
			{
				return operator new(size);
			}

			// 相当于size / 8 向上取整（因为分配内存只能大不能小
			return getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).allocate();
		}

		static void freeMemory(void* ptr, size_t size)
		{
			if (!ptr)
			{
				return;
			}
			if (size > MAX_SLOT_SIZE)
			{
				operator delete(ptr);
				return;
			}

			getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).deallocate(ptr);
		}

		template<typename T, typename... Args>
		friend T* newElement(Args&&... args);

		template<typename T>
		friend void deleteElement(T* p);
	};

	template<typename T, typename... Args>
	T* newElement(Args&&... args)
	{
		T* p = nullptr;
		// 根据元素大小选择合适的内存池分配内存
		if ((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T))))!= nullptr)
		{
			// 在分配的内存上构造对象
			new(p) T(std::forward<Args>(args)...);
		}

		return p;
	}

	template<typename T>
	void deleteElement(T* p)
	{
		if (p)
		{
			p->~T();
			// 内存回收
			HashBucket::freeMemory(reinterpret_cast<void*>(p), sizeof(T));
		}
	}

}