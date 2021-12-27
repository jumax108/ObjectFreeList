#ifndef __OBJECT_FREE_LIST_TLS__

#define __OBJECT_FREE_LIST_TLS__

//#define OBJECT_FREE_LIST_TLS_SAFE
//#define OBJECT_FREE_LIST_TLS_LOG

#include "ObjectFreeList.h"
#include "SimpleProfiler.h"

extern SimpleProfiler sp;

// T type data �ּҿ� �� ���� ���ϸ� node �ּҰ� �˴ϴ�.
static constexpr __int64 _dataToNodePtr = -8;

template <typename T>
struct stAllocChunk;

template <typename T>
struct stAllocTlsNode{
	
	unsigned __int64 _underflowCheck;

	T _data;

	unsigned __int64 _overflowCheck;

	// ��尡 �ҼӵǾ��ִ� ûũ
	stAllocChunk<T>* _afflicatedChunk;

	stAllocTlsNode(){

	}
	stAllocTlsNode(stAllocChunk<T>* afflicatedChunk){
		
		_underflowCheck = 0xD3D3D3D3D3D3D3D3;
		_overflowCheck	= 0xD3D3D3D3D3D3D3D3;

		_afflicatedChunk = afflicatedChunk;

	}

};

template <typename T>
struct stAllocChunk{
		
public:
 	alignas(64) unsigned __int64 _underflowCheck;

	stAllocTlsNode<T>* _nodes;

	unsigned __int64 _overflowCheck;
	
	int _nodeNum;

	int _leftNodeCnt;
	int _leftFreeCnt;


	stAllocChunk(){
		
		_underflowCheck = 0xD3D3D3D3D3D3D3D3;
		_overflowCheck	= 0xD3D3D3D3D3D3D3D3;

		_nodeNum = 100;

		_nodes = new stAllocTlsNode<T>[_nodeNum];

		for(int nodeCnt = 0; nodeCnt < _nodeNum; ++nodeCnt){
			new (&_nodes[nodeCnt]) stAllocTlsNode<T>(this);
		}

		_leftNodeCnt = _nodeNum;
		_leftFreeCnt = _nodeNum;

	}

	~stAllocChunk(){
		
		_leftNodeCnt = _nodeNum;
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
	
	struct stLogLine{

		unsigned char code;
		unsigned int threadID;
		unsigned int logCnt;

		void* allocChunk;
		void* allocNode;
		unsigned __int64 leftNodeCnt;

		void* freeChunk;
		void* freeNode;
		unsigned __int64 leftFreeCnt;

	};

	unsigned __int64 NO_LOG_DATA = 0xFFFFFFFFFFFFFFFF;
	void* NO_LOG_PTR = (void*)0xFFFFFFFFFFFFFFFF;
	stLogLine _log[65536];
	unsigned __int64 _logCnt;

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

	chunk->_leftNodeCnt -= 1;
	int allocNodeIdx = chunk->_leftNodeCnt;

	T* allocData = &chunk->_nodes[allocNodeIdx]._data;

	if(_runConstructor == true){
		new (allocData) T();
	}

	if(allocNodeIdx == 0){
		TlsSetValue(_allocChunkTlsIdx, nullptr);
	}

	return allocData;

}

template <typename T>
void CObjectFreeListTLS<T>::freeObject(T* object){

	if(_runDestructor == true){
		object->~T();
	}
	stAllocTlsNode<T>* node = ((stAllocTlsNode<T>*)((unsigned __int64)object + _dataToNodePtr));
	stAllocChunk<T>* chunk = node->_afflicatedChunk;
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