#pragma once
template<typename T>
class CObjectFreeList
{
public:

	struct stNode {
		stNode() {
			nextNode = nullptr;
		}
		T data;
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
		stNode* ptr;
		stAllocList* nextNode;
	};

	stNode* _freeNode;

	unsigned int _capacity;

	unsigned int _usedCnt;

	stAllocList* _allocNodeHead;

};

template <typename T>
CObjectFreeList<T>::CObjectFreeList() {

	_freeNode = nullptr;
	_allocNodeHead = nullptr;

}

template <typename T>
CObjectFreeList<T>::~CObjectFreeList() {

	while (_allocNodeHead != nullptr) {
		stAllocList* nextNode = _allocNodeHead->nextNode;
		delete _allocNodeHead->ptr;
		delete _allocNodeHead;
		_allocNodeHead = nextNode;
	}

}

template<typename T>
T* CObjectFreeList<T>::alloc() {

	_usedCnt += 1;
	if (_freeNode == nullptr) {

		CObjectFreeList<T>::stNode* node = new CObjectFreeList<T>::stNode;

		_capacity += 1;

		stAllocList* listHead = _allocNodeHead;
		_allocNodeHead = new stAllocList();
		_allocNodeHead->ptr = node;
		_allocNodeHead->nextNode = listHead;

		return &node->data;

	}

	T* allocNode = &_freeNode->data;
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