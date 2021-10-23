#pragma once
template<typename T>
class CObjectFreeList
{
public:

	struct stNode {
		stNode() {
			data = nullptr;
			nextNode = nullptr;
		}
		T* data;
		stNode* nextNode;
	};

	CObjectFreeList();
	~CObjectFreeList();

	T* alloc();

	void free(T* data);

	inline unsigned int getCapacity() { return _capacity; }
	inline unsigned int getUsedCount() { return _usedCnt; }

private:

	struct stAllocList {
		void* ptr;
		stAllocList* nextNode;
	};

	stNode* _freeNode;

	unsigned int _capacity;

	unsigned int _usedCnt;

	stAllocList* _allocList;

};

template <typename T>
CObjectFreeList<T>::CObjectFreeList() {

	_freeNode = nullptr;

}

template <typename T>
CObjectFreeList<T>::~CObjectFreeList() {



}

template<typename T>
T* CObjectFreeList<T>::alloc() {

	_usedCnt += 1;
	if (_freeNode == nullptr) {

		CObjectFreeList<T>::stNode* node = new CObjectFreeList<T>::stNode;
		node->data = new T;

		_capacity += 1;

		return (T*)node;

	}

	T* allocNode = (T*)_freeNode;
	_freeNode = _freeNode->nextNode;
	new (allocNode) T();
	return allocNode;
}

template <typename T>
void CObjectFreeList<T>::free(T* data) {

	((stNode*)data)->nextNode = _freeNode;
	_freeNode = (stNode*)data;
	_usedCnt -= 1;
	data->~T();

}