#pragma once

class CAllocList;

template<typename T>
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

	// f9�� �ʱ�ȭ�ؼ� ����÷ο� üũ�մϴ�.
	void* underFlowCheck;

	// alloc �Լ����� ������ ���� ������
	T data;

	// f9�� �ʱ�ȭ�ؼ� �����÷ο� üũ�մϴ�.
	void* overFlowCheck;

	// alloc �Լ����� �Ҵ� ������ �ϱ� ����
	stNode<T>* nextNode;

	// allocList�� ����� �ּҰ��� ������ִ� �迭�� �ּҰ��Դϴ�.
	// �迭 �����ص� ���� �����ǵ��� �迭�� �ּҸ� �����մϴ�.
	// �ֻ�� ��Ʈ�� 1�̸� alloc �Լ��� ���� �Ҵ�� �����Դϴ�.
	void** allocListPtr;
};

template<typename T>
class CObjectFreeList
{
public:

	CObjectFreeList(int _capacity = 0);
	~CObjectFreeList();

	T* alloc();

	int free(T* data);

	inline unsigned int getCapacity() { return _capacity; }
	inline unsigned int getUsedCount() { return _usedCnt; }

private:


	// ��� ������ ��带 ����Ʈ�� ���·� �����մϴ�.
	// �Ҵ��ϸ� �����մϴ�.
	stNode<T>* _freeNode;

	// ��ü ��� ����
	unsigned int _capacity;

	// ���� �Ҵ�� ��� ����
	unsigned int _usedCnt;

	// ������ ������(stNode->data)�� �� ���� ���ϸ� ��� ������(stNode)�� �ȴ� !
	int _dataPtrToNodePtr;

	// �Ҵ� ���� �����͸� �����ϴ� �迭
	// �ε����� ������Ʈ 20���� ����մϴ�.
	// ���⼭ ��� �ϳ� �ϳ��� �����͸� �����մϴ�.
	CAllocList* _ptrList;

	// ���� ��Ʈ�� BIT_NUM_TO_INDEX�� ������ŭ �̱����� ����ũ
	int _mask;

	// ���� ��Ʈ ��� �ε����� ������� ����
	#ifdef _WIN64
		const unsigned short BIT_NUM_TO_INDEX = 20;
	#else
		const unsigned short BIT_NUM_TO_INDEX = 10;
	#endif

	// �޸� ������
	// �ܼ� ����Ʈ
	struct stSimpleListNode {
		void* ptr;
		stSimpleListNode* next;
	};

	// �Ʒ� 2�� List�� new�� ����� ���޵� �����͵�θ� �����Ǿ� �ֽ��ϴ�.
	// new stNode[100] �̸� node[0]�� ����Ǿ��ִ� ���Դϴ�.
	// freeList �Ҹ��ڿ��� �޸� ���������� ����մϴ�.
	 
	// ���Ϸ� new�� �����͵�
	stSimpleListNode* allocList;
	// �迭�� new�� �����͵�
	stSimpleListNode* arrAllocList;

	// �����͸� memoryMangeArr�� �ε����� ������
	int ptrToIndex(void*);

	// ������ stNode �� �����쿡�� �Ҵ�޴� ���Դϴ�.
	// �迭�� �ƴ� ���� ��ü�� �Ҵ���� �� ����մϴ�.
	stNode<T>* actualSingleAlloc();

	// ������ stNode �� �����쿡�� �Ҵ�޴� ���Դϴ�.
	// �迭�� �Ҵ�޽��ϴ�.
	stNode<T>* actualArrayAlloc(int size);

};

class CAllocList {

public:

	template<typename T>
	void push(void* data);

	inline void** begin() { return arr; }
	inline void** end() { return arr + size; }

	// ������ 2��� ����
	template<typename T>
	void resize();

	CAllocList();
	~CAllocList();

private:

	void** arr;
	unsigned int cap;
	unsigned int size;

};

CAllocList::CAllocList() {
	cap = 10;
	size = 0;
	arr = new void* [cap];

}

CAllocList::~CAllocList() {
	delete[] arr;
}

template<typename T>
void CAllocList::push(void* data) {
	if (size >= cap) {
		resize<T>();
	}
	arr[size] = data;
	((stNode<T>*)data)->allocListPtr = &arr[size];
	size += 1;
}

template<typename T>
void CAllocList::resize() {
	void** oldArr = arr;

	cap *= 2;
	arr = new void* [cap];

	for (int arrCnt = 0; arrCnt < size; ++arrCnt) {
		arr[arrCnt] = oldArr[arrCnt];
		((stNode<T>*)(arr[arrCnt]))->allocListPtr = &arr[arrCnt];
	}

	delete[](oldArr);
}

template <typename T>
CObjectFreeList<T>::CObjectFreeList(int size) {

	allocList = nullptr;
	arrAllocList = nullptr;
	_freeNode = nullptr;

	_capacity = size;
	_usedCnt = 0;

	if (size == 0) {
		return;
	}

	_ptrList = new CAllocList[BIT_NUM_TO_INDEX];
	for (int bitCnt = 0; bitCnt < BIT_NUM_TO_INDEX - 1; ++bitCnt) {
		_mask = (_mask + 1) << 1;
	}
	_mask += 1;

	stNode<T>* nodeArr = actualArrayAlloc(_capacity);

	stNode<T>* nodeEnd = nodeArr + size;
	for (stNode<T>* nodeIter = nodeArr;;) {

		stNode<T>* node = nodeIter;
		++nodeIter;

		int index = ptrToIndex(node);
		_ptrList[index].push<T>(node);

		if (nodeIter == nodeEnd) {
			break;
		}

		node->nextNode = nodeIter;
	}
	_freeNode = nodeArr;

	stNode<T> node;
	_dataPtrToNodePtr = ((char*)&node) - ((char*)&node.data);

}

template <typename T>
CObjectFreeList<T>::~CObjectFreeList() {

	while (allocList != nullptr) {
		stSimpleListNode* nextNode = allocList->next;
		delete (stNode<T>*)(allocList->ptr);
		delete allocList;
		allocList = nextNode;
	}

	while (arrAllocList != nullptr) {
		stSimpleListNode* nextNode = arrAllocList->next;
		delete[] (stNode<T>*)(arrAllocList->ptr);
		delete arrAllocList;
		arrAllocList = nextNode;
	}

	delete[] _ptrList;

}

template<typename T>
T* CObjectFreeList<T>::alloc() {

	_usedCnt += 1;
	if (_freeNode == nullptr) {

		stNode<T>* node = actualSingleAlloc();

		_capacity += 1;

		int index = ptrToIndex(node);
		_ptrList[index].push<T>(node);

		return &node->data;

	}

	T* allocNode = &_freeNode->data;

	#ifdef _WIN64
		unsigned __int64 ptrTemp = (unsigned __int64)(*(_freeNode->allocListPtr));
		ptrTemp |= 0x8000000000000000;
	#else
		unsigned int ptrTemp = (unsigned int)(*(_freeNode->allocListPtr));
		ptrTemp |= 0x80000000;
	#endif

	*(_freeNode->allocListPtr) = (void*)ptrTemp;
	_freeNode = _freeNode->nextNode;

	new (allocNode) T();
	return allocNode;
}

template <typename T>
int CObjectFreeList<T>::free(T* data) {

	stNode<T>* usedNode = (stNode<T>*)(((char*)data) + _dataPtrToNodePtr);

#ifdef _WIN64
	
	if (usedNode->underFlowCheck != (void*)0xF9F9F9F9F9F9F9F9) {
		// underflow
		return -1;
	}
	if (usedNode->overFlowCheck != (void*)0xF9F9F9F9F9F9F9F9) {
		// overflow
		return 1;
	}
	if (((unsigned __int64)(*(usedNode->allocListPtr)) & 0x8000000000000000) == 0) {
		// �ߺ� free
		return -2;
	}
	unsigned __int64 ptrTemp = (unsigned __int64)(*(usedNode->allocListPtr));
	ptrTemp &= 0x7FFFFFFFFFFFFFFF;
	*(usedNode->allocListPtr) = (void*)ptrTemp;
#else
	if (usedNode->underFlowCheck != (void*)0xF9F9F9F9) {
		// underflow
		return -1;
	}
	if (usedNode->overFlowCheck != (void*)0xF9F9F9F9) {
		// overflow
		return 1;
	}
	if (((unsigned int)(*(usedNode->allocListPtr)) & 0x80000000) == 0) {
		// �ߺ� free
		return -2;
	}
	unsigned int ptrTemp = (unsigned int)(*(usedNode->allocListPtr));
	ptrTemp &= 0x7FFFFFFF;
	*(usedNode->allocListPtr) = (void*)ptrTemp;
#endif


	usedNode->nextNode = _freeNode;

	_freeNode = usedNode;
	_usedCnt -= 1;
	data->~T();

	return 0;
}

template<typename T>
int CObjectFreeList<T>::ptrToIndex(void* ptr) {

	int index = 0;

	int lowBit = (int)ptr & _mask;

	while (lowBit > 0) {
		index += lowBit & 1;
		lowBit >>= 1;
	}

	return index;

}

template<typename T>
stNode<T>* CObjectFreeList<T>::actualSingleAlloc() {

	stNode<T>* node = new stNode<T>;

	stSimpleListNode* allocNode = new stSimpleListNode;
	allocNode->ptr = node;
	allocNode->next = allocList;
	allocList = allocNode;

	return node;
}

template<typename T>
stNode<T>* CObjectFreeList<T>::actualArrayAlloc(int size) {

	stNode<T>* nodeArr = new stNode<T>[size];

	stSimpleListNode* arrAllocNode = new stSimpleListNode;
	arrAllocNode->ptr = nodeArr;
	arrAllocNode->next = arrAllocList;
	arrAllocList = arrAllocNode;

	return nodeArr;
}