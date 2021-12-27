#ifndef __OBJECT_FREE_LIST_TLS__

#define __OBJECT_FREE_LIST_TLS__

//#define OBJECT_FREE_LIST_TLS_SAFE
//#define OBJECT_FREE_LIST_TLS_LOG

#include "ObjectFreeList.h"
#include "SimpleProfiler.h"

extern SimpleProfiler sp;

// T type data �ּҿ� �� ���� ���ϸ� node �ּҰ� �˴ϴ�.
static constexpr __int64 _dataToNodePtr = 0;

template <typename T>
struct stAllocChunk;

template <typename T>
struct stAllocTlsNode{
	
	T _data;

	// ��尡 �ҼӵǾ��ִ� ûũ
	stAllocChunk<T>* _afflicatedChunk;

	stAllocTlsNode(){

	}
	stAllocTlsNode(stAllocChunk<T>* afflicatedChunk){
		
		_afflicatedChunk = afflicatedChunk;

	}

};

template <typename T>
struct stAllocChunk{
		
public:

	stAllocTlsNode<T>* _nodes;	
	stAllocTlsNode<T>* _allocNode;
	stAllocTlsNode<T>* _nodeEnd;

	int _nodeNum;

	int _leftFreeCnt;

	stAllocChunk(){
		
		_nodeNum = 100;

		_nodes = new stAllocTlsNode<T>[_nodeNum];

		_allocNode = _nodes;
		_nodeEnd   = _nodes + _nodeNum;

		_leftFreeCnt = _nodeNum;

		for(int nodeCnt = 0; nodeCnt < _nodeNum; ++nodeCnt){
			new (&_nodes[nodeCnt]) stAllocTlsNode<T>(this);
		}

	}

	~stAllocChunk(){

		_allocNode = _nodes;
		_leftFreeCnt = _nodeNum;

	}

};

template <typename T>
class CObjectFreeListTLS{

public:

	CObjectFreeListTLS(HANDLE heap, bool runConstructor, bool runDestructor);

	T*	allocObject();
	void freeObject(T* object);

	unsigned int getCapacity();
	unsigned int getUsedCount();
	
private:
	
	// ��� �����尡 ���������� �����ϴ� free list �Դϴ�.
	// �̰����� T type�� node�� ū ����� ���ɴϴ�.
	CObjectFreeList<stAllocChunk<T>>* _centerFreeList;

	// heap
	HANDLE _heap;

	// �� �����忡�� Ȱ���� ûũ�� ����ִ� tls�� index
	unsigned int _allocChunkTlsIdx;
		
	// T type�� ���� ������ ȣ�� ����
	bool _runConstructor;

	// T type�� ���� �Ҹ��� ȣ�� ����
	bool _runDestructor;

};

template <typename T>
CObjectFreeListTLS<T>::CObjectFreeListTLS(HANDLE heap, bool runConstructor, bool runDestructor){

	_heap = heap;

	_centerFreeList = (CObjectFreeList<stAllocChunk<T>>*)HeapAlloc(_heap, 0, sizeof(CObjectFreeList<stAllocChunk<T>>));
	new (_centerFreeList) CObjectFreeList<stAllocChunk<T>>(_heap, false, true);

	_allocChunkTlsIdx = TlsAlloc();
	if(_allocChunkTlsIdx == TLS_OUT_OF_INDEXES){
		CDump::crash();
	}

	stAllocTlsNode<T> tempNode;
	if(_dataToNodePtr != (unsigned __int64)&tempNode - (unsigned __int64)&tempNode._data){
		CDump::crash();
	}

	_runConstructor = runConstructor;
	_runDestructor	= runDestructor;


}

template <typename T>
typename T* CObjectFreeListTLS<T>::allocObject(){

	stAllocChunk<T>* chunk = (stAllocChunk<T>*)TlsGetValue(_allocChunkTlsIdx);
	
	if(chunk == nullptr){
		chunk = _centerFreeList->allocObject();
		TlsSetValue(_allocChunkTlsIdx, chunk);
	}

	T* allocData = &chunk->_allocNode++->_data;

	if(_runConstructor == true){
		new (allocData) T();
	}

	if(chunk->_allocNode == chunk->_nodeEnd){
		TlsSetValue(_allocChunkTlsIdx, nullptr);
	}

	return allocData;

}

template <typename T>
void CObjectFreeListTLS<T>::freeObject(T* object){

	if(_runDestructor == true){
		object->~T();
	}
	stAllocChunk<T>* chunk = ((stAllocTlsNode<T>*)((unsigned __int64)object + _dataToNodePtr))->_afflicatedChunk;
	chunk->_leftFreeCnt -= 1;
	if(chunk->_leftFreeCnt == 0){
		_centerFreeList->freeObject(chunk);
	}
}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getUsedCount(){
	
	return _centerFreeList->getUsedCount();

}

template <typename T>
unsigned int CObjectFreeListTLS<T>::getCapacity(){
	
	return _centerFreeList->getCapacity();

}

#endif