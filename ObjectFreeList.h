#pragma once
template<typename T>
class CObjectFreeList
{
public:

	struct stNode {
		stNode() {
			nextNode = nullptr;
#ifdef _WIN64
			underFlowCheck = (void*)0xF9F9F9F9F9F9F9F9;
			overFlowCheck = (void*)0xF9F9F9F9F9F9F9F9;
#else
			underFlowCheck = (void*)0xF9F9F9F9;
			overFlowCheck = (void*)0xF9F9F9F9;
#endif
		}
		void* underFlowCheck;
		T data;
		void* overFlowCheck;
		stNode* nextNode;
	};

	CObjectFreeList(int _capacity = 0);
	~CObjectFreeList();

	T* alloc();

	int free(T* data);

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

	// 데이터 포인트에 이 값을 더하면 노드 포인터가 된다 !
	int dataPtrToNodePtr;
};

template <typename T>
CObjectFreeList<T>::CObjectFreeList(int size) {

	_freeNode = nullptr;
	_allocNodeHead = nullptr;

	_capacity = size;
	_usedCnt = 0;

	if (size == 0) {
		return;
	}

	stNode* nodeArr = new stNode[size];
	for (int nodeCnt = 0; nodeCnt < size - 1; ++nodeCnt) {
		nodeArr[nodeCnt].nextNode = &nodeArr[nodeCnt + 1];
	}
	_freeNode = nodeArr;

	stNode node;
	dataPtrToNodePtr = ((char*)&node) - ((char*)&node.data);
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
int CObjectFreeList<T>::free(T* data) {

	stNode* usedNode = (stNode*)(((char*)data) + dataPtrToNodePtr);

#ifdef _WIN64
	if (usedNode->underFlowCheck != (void*)0xF9F9F9F9F9F9F9F9) {
		// underflow
		return -1;
	}
	if (usedNode->overFlowCheck != (void*)0xF9F9F9F9F9F9F9F9) {
		// overflow
		return 1;
	}
#else
	if (usedNode->underFlowCheck != (void*)0xF9F9F9F9) {
		// underflow
		return -1;
	}
	if (usedNode->overFlowCheck != (void*)0xF9F9F9F9) {
		// overflow
		return 1;
	}
#endif

	usedNode->nextNode = _freeNode;
	_freeNode = usedNode;
	_usedCnt -= 1;
	data->~T();

	return 0;
}
