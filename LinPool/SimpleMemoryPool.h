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

		// CAS����������Ӻͳ���
		bool pushFreeList(Slot* slot);
		Slot* popFreeList();

	private:
		int BlockSize_; // �ڴ���С
		int SlotSize_; // ÿ��Slot�Ĵ�С
		Slot* firstBlock_; // ָ���ڴ�ع�����׸�ʵ���ڴ��
		Slot* curSlot_; // ָ��ǰδ��ʹ�ù��Ĳ�
		std::atomic<Slot*> freeList_; // ָ����в������ͷ��
		Slot* lastSlot_; // ��Ϊ��ǰ�ڴ��������ܹ����Ԫ�ص�λ�ñ�ʶ(������λ���������µ��ڴ��)
		std::mutex mutexForBlock_; // ��֤���߳�����±��ⲻ��Ҫ���ظ������ڴ浼�µ��˷���Ϊ��

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

			// �൱��size / 8 ����ȡ������Ϊ�����ڴ�ֻ�ܴ���С
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
		// ����Ԫ�ش�Сѡ����ʵ��ڴ�ط����ڴ�
		if ((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T))))!= nullptr)
		{
			// �ڷ�����ڴ��Ϲ������
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
			// �ڴ����
			HashBucket::freeMemory(reinterpret_cast<void*>(p), sizeof(T));
		}
	}

}