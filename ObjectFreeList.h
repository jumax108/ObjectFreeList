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

	// f9로 초기화해서 언더플로우 체크합니다.
	void* underFlowCheck;

	// alloc 함수에서 리턴할 실제 데이터
	T data;

	// f9로 초기화해서 오버플로우 체크합니다.
	void* overFlowCheck;

	// alloc 함수에서 할당 빠르게 하기 위함
	stNode<T>* nextNode;

	// allocList에 노드의 주소값이 저장되있는 배열의 주소값입니다.
	// 배열 수정해도 같이 수정되도록 배열의 주소를 저장합니다.
	// 최상단 비트가 1이면 alloc 함수를 통해 할당된 상태입니다.
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


	// 사용 가능한 노드를 리스트의 형태로 저장합니다.
	// 할당하면 제거합니다.
	stNode<T>* _freeNode;

	// 전체 노드 개수
	unsigned int _capacity;

	// 현재 할당된 노드 개수
	unsigned int _usedCnt;

	// 데이터 포인터(stNode->data)에 이 값을 더하면 노드 포인터(stNode)가 된다 !
	int _dataPtrToNodePtr;

	// 할당 받은 포인터를 저장하는 배열
	// 인덱스는 하위비트 20개로 사용합니다.
	// 여기서 노드 하나 하나의 포인터를 관리합니다.
	CAllocList* _ptrList;

	// 하위 비트를 BIT_NUM_TO_INDEX의 개수만큼 뽑기위한 마스크
	int _mask;

	// 하위 비트 몇개를 인덱스로 사용할지 결정
	#ifdef _WIN64
		const unsigned short BIT_NUM_TO_INDEX = 20;
	#else
		const unsigned short BIT_NUM_TO_INDEX = 10;
	#endif

	// 메모리 정리용
	// 단순 리스트
	struct stSimpleListNode {
		void* ptr;
		stSimpleListNode* next;
	};

	// 아래 2개 List는 new의 결과로 전달된 포인터들로만 구성되어 있습니다.
	// new stNode[100] 이면 node[0]만 저장되어있는 식입니다.
	// freeList 소멸자에서 메모리 정리용으로 사용합니다.
	 
	// 단일로 new한 포인터들
	stSimpleListNode* allocList;
	// 배열로 new한 포인터들
	stSimpleListNode* arrAllocList;

	// 포인터를 memoryMangeArr의 인덱스로 변경함
	int ptrToIndex(void*);

	// 실제로 stNode 를 윈도우에서 할당받는 곳입니다.
	// 배열이 아닌 단일 객체를 할당받을 때 사용합니다.
	stNode<T>* actualSingleAlloc();

	// 실제로 stNode 를 윈도우에서 할당받는 곳입니다.
	// 배열로 할당받습니다.
	stNode<T>* actualArrayAlloc(int size);

};

class CAllocList {

public:

	template<typename T>
	void push(void* data);

	inline void** begin() { return arr; }
	inline void** end() { return arr + size; }

	// 무조건 2배로 증가
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
		// 중복 free
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
		// 중복 free
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