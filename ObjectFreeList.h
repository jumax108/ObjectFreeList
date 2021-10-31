#pragma once

class CAllocList;

#define allocObject() _allocObject(__FILEW__, __LINE__)
#define freeObject(x) _freeObject(x, __FILEW__, __LINE__)

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

	// �ҽ� ���� �̸�
	const wchar_t* allocSourceFileName;
	const wchar_t* freeSourceFileName;

	// �ҽ� ����
	int allocLine;
	int freeLine;
};

template<typename T>
class CObjectFreeList
{
public:

	CObjectFreeList(int _capacity = 0);
	~CObjectFreeList();

	T* _allocObject(const wchar_t*, int);

	int _freeObject(T* data, const wchar_t*, int);

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
	// ���⼭ ��� �ϳ� �ϳ��� �����͸� �����մϴ�.
	CAllocList* _ptrList;


	// �޸� ������
	// �ܼ� ����Ʈ
	struct stSimpleListNode {
		void* ptr;
		stSimpleListNode* next;
	};

	// �Ʒ� 2�� List�� new�� ����� ���޵� �����͵�θ� �����Ǿ� �ֽ��ϴ�.
	// new stNode[100] �̸� node[0]�� ����Ǿ��ִ� ���Դϴ�.
	// freeList �Ҹ��ڿ��� �޸� ���������� ����մϴ�.
	 
	// �迭�� new�� �����͵�
	stSimpleListNode* arrAllocList;

	// �����͸� memoryMangeArr�� �ε����� ������
	int ptrToIndex(void*);

	// ������ stNode �� �����쿡�� �Ҵ�޴� ���Դϴ�.
	// �迭�� �Ҵ�޽��ϴ�.
	stNode<T>* actualArrayAlloc(int size);

	// ������Ʈ�� 2��� �Ҵ��մϴ�.
	void resize();
};

class CAllocList {

public:

	template<typename T>
	void push(void* data);

	inline void** begin() { return _arr; }
	inline void** end() { return _arr + _size; }

	// ������ 2��� ����
	template<typename T>
	void resize();

	CAllocList(unsigned int cap = 10);
	~CAllocList();

	template<typename T>
	friend class CObjectFreeList;

private:

	void** _arr;
	unsigned int _cap;
	unsigned int _size;

};

CAllocList::CAllocList(unsigned int cap) {
	_cap = 10;
	_size = 0;
	_arr = new void* [cap];

}

CAllocList::~CAllocList() {
	delete[] _arr;
}

template<typename T>
void CAllocList::push(void* data) {
	if (_size >= _cap) {
		resize<T>();
	}
	_arr[size] = data;
	((stNode<T>*)data)->allocListPtr = &_arr[size];
	_size += 1;
}

template<typename T>
void CAllocList::resize() {
	void** oldArr = _arr;

	_cap *= 2;
	_arr = new void* [_cap];

	for (int arrCnt = 0; arrCnt < _size; ++arrCnt) {
		_arr[arrCnt] = oldArr[arrCnt];
		((stNode<T>*)(_arr[arrCnt]))->allocListPtr = &_arr[arrCnt];
	}

	delete[](oldArr);
}

template <typename T>
CObjectFreeList<T>::CObjectFreeList(int size) {

	arrAllocList = nullptr;
	_freeNode = nullptr;

	_capacity = size;
	_usedCnt = 0;

	if (size == 0) {
		return;
	}

	_ptrList = new CAllocList(_capacity);

    actualArrayAlloc(_capacity);
	stNode<T> node;
	_dataPtrToNodePtr = ((char*)&node) - ((char*)&node.data);

}

template <typename T>
CObjectFreeList<T>::~CObjectFreeList() {

	for(int allocCnt = 0; allocCnt < _ptrList[allocListCnt].size; ++allocCnt){
		stNode<T>* node = (stNode<T>*)_ptrList[allocListCnt].arr[allocCnt];

		#if defined(_WIN64)
			if ((unsigned __int64)node & 0x8000000000000000) {
				unsigned __int64 temp = (unsigned __int64)node;
				temp &= 0x7FFFFFFFFFFFFFF;
				node = (stNode<T>*)temp;
				wprintf(L"�޸� ����\n ptr: 0x%016I64x\n allocFile: %s\n allocLine: %d\n", &node->data, node->allocSourceFileName, node->allocLine);
			}
		#else
			if ((unsigned int)node & 0x80000000) {
				unsigned int temp = (unsigned int)node;
				temp &= 0x7FFFFFFF;
				node = (stNode<T>*)temp;
				wprintf(L"�޸� ����\n ptr: 0x%08x\n allocFile: %s\n allocLine: %d\n", &node->data, node->allocSourceFileName, node->allocLine);
			}
		#endif

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
T* CObjectFreeList<T>::_allocObject(const wchar_t* fileName, int line) {

	_usedCnt += 1;
	if (_freeNode == nullptr) {

		resize();

		wprintf(L"ObjectFreeList, resize()\nfileName: %s\nline: %d\n", fileName, line);

	}

	stNode<T>* allocNode = _freeNode;

	#ifdef _WIN64
		unsigned __int64 ptrTemp = (unsigned __int64)(*(allocNode->allocListPtr));
		ptrTemp |= 0x8000000000000000;
	#else
		unsigned int ptrTemp = (unsigned int)(*(_freeNode->allocListPtr));
		ptrTemp |= 0x80000000;
	#endif

	*(allocNode->allocListPtr) = (void*)ptrTemp;

	allocNode->allocSourceFileName = fileName;
	allocNode->allocLine = line;

	_freeNode = _freeNode->nextNode;

	new (&(allocNode->data)) T();

	return &(allocNode->data);
}

template <typename T>
int CObjectFreeList<T>::_freeObject(T* data, const wchar_t* fileName, int line) {

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

	usedNode->freeSourceFileName = fileName;
	usedNode->freeLine = line;

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
stNode<T>* CObjectFreeList<T>::actualArrayAlloc(int size) {

	stNode<T>* nodeArr = new stNode<T>[size];

	stSimpleListNode* arrAllocNode = new stSimpleListNode;
	arrAllocNode->ptr = nodeArr;
	arrAllocNode->next = arrAllocList;
	arrAllocList = arrAllocNode;

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

	return nodeArr;
}

template<typename T>
void CObjectFreeList<T>::resize(){
	
	actualArrayAlloc(_capacity);

	_capacity <<= 1;

}