#include <Windows.h>
#include <stdio.h>
#include <new>
#include <locale>
#include <DbgHelp.h>
#include <thread>

#include "lib/profiler/headers/profiler.h"
#pragma comment(lib, "lib/profiler/profiler")

#include "../headers/ObjectFreeList.h"

#define LOGIC_TEST

CDump dump;

void* const nodeStartValue = (void*)0x1122330044556677;
void* const nodeEndValue   = (void*)0x8899AABBCCDDEEFF;

struct stNode{

	void* nodeStart;
	__int64 data;
	void* nodeEnd;

};

#ifdef LOGIC_TEST
constexpr int ALLOC_NUM_EACH_THREAD = 1000;
constexpr int THREAD_NUM = 3;
constexpr int MAX_ALLOC_NUM = ALLOC_NUM_EACH_THREAD * THREAD_NUM;
CObjectFreeList<stNode>* nodeFreeList;
unsigned int tps = 0;

unsigned __stdcall logicTestThreadFunc(void* arg){

	/*
	* 
	* 테스트 시나리오
	* 
	* 1. 노드를 ALLOC_NUM_EACH_THREAD 만큼 할당받는다.
	* 2. 노드의 모든 데이터가 초기값과 일치하는지 확인한다. -> 다른 스레드가 사용중인 주소를 할당받는지 확인, 정상적인 주소를 받는지 확인
	* 3. 할당받은 모든 노드의 Data를 1 증가한다.
	* 4. 모든 노드의 Data가 1인지 확인한다.                 -> 다른 스레드가 사용중인 주소를 사용중이지 않는지 확인
	* 5. 모든 노드의 Data를 1 감소한다.                     
	* 6. 모든 노드의 Data가 0인지 확인한다.                 -> 다른 스레드가 사용중인 주소를 사용중이지 않는지 확인
	* 7. 노드를 ALLOC_NUM_EACH_THREAD 만큼 해제한다.
	* 8. capacity가 MAX 값을 넘어가지 않았는지 확인한다.    -> 중간에 노드가 사라지지 않았는지 확인
	* 
	*/

	srand((unsigned int)(LONG64)arg);

	stNode** nodeArr = new stNode*[ALLOC_NUM_EACH_THREAD];

	HANDLE thread = *(HANDLE*)arg;

	while(1){

		// 1. 노드를 ALLOC_NUM_EACH_THREAD 만큼 할당받는다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){
			nodeArr[nodeCnt] = nodeFreeList->allocObject();
		}
		Sleep(0);

		// 2. 노드의 모든 데이터가 초기값과 일치하는지 확인한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){
			stNode* node = nodeArr[nodeCnt];
		
			if(node->nodeStart != nodeStartValue){
				CDump::crash();
			}

			if(node->nodeEnd != nodeEndValue){
				CDump::crash();
			}

			if(node->data != 0){
				CDump::crash();
			}

		}		 
		
		// 3. 할당받은 모든 노드의 Data를 1 증가한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){

			stNode* node = nodeArr[nodeCnt];
			InterlockedIncrement64((LONG64*)&node->data);

		}


		// 4. 모든 노드의 Data가 1인지 확인한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){
		
			stNode* node = nodeArr[nodeCnt];
			if(node->data != 1){
				CDump::crash();
			}

		}
		
		
		// 5. 모든 노드의 Data를 1 감소한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){

			stNode* node = nodeArr[nodeCnt];
			InterlockedDecrement64((LONG64*)&node->data);

		}
		
		// 6. 모든 노드의 Data가 0인지 확인한다. 
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++ nodeCnt){
		
			stNode* node = nodeArr[nodeCnt];
			if(node->data != 0){
				CDump::crash();
			}

		}

		// 7. 노드를 ALLOC_NUM_EACH_THREAD 만큼 해제한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){

			stNode* node = nodeArr[nodeCnt];
			nodeFreeList->freeObject(node);

		}
		
		// 8. capacity가 MAX 값을 넘어가지 않았는지 확인한다.
		unsigned int usedCnt = nodeFreeList->getUsedCount();
		unsigned int capacity = nodeFreeList->getCapacity();
		if(capacity > MAX_ALLOC_NUM){
		//	CDump::crash();
		}

		InterlockedIncrement(&tps);
	}

	return 0;

}

void logicTest() {

	nodeFreeList = new CObjectFreeList<stNode>(false, false);

	stNode** nodeArr = new stNode*[MAX_ALLOC_NUM];

	// 노드 초기화
	for(int nodeCnt = 0; nodeCnt < MAX_ALLOC_NUM; ++nodeCnt){
		
		nodeArr[nodeCnt] = nodeFreeList->allocObject();
		stNode* node = nodeArr[nodeCnt];	

		node->nodeStart = nodeStartValue;
		node->data = 0;
		node->nodeEnd = nodeEndValue;
	}

	for(int nodeCnt = 0 ; nodeCnt < MAX_ALLOC_NUM; ++nodeCnt){
		stNode* node = nodeArr[nodeCnt];
		nodeFreeList->freeObject(node);		
	}
	
	HANDLE thread[THREAD_NUM];
	for(int threadCnt = 0; threadCnt < THREAD_NUM; ++threadCnt){

		thread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, logicTestThreadFunc, (void*)&thread[threadCnt], 0, nullptr);

	}

	while(1){
	
		unsigned int usedCnt = nodeFreeList->getUsedCount();
		unsigned int capacity = nodeFreeList->getCapacity();
		wprintf(L"tps: %d, usedCnt: %d, capacity: %d\n", tps, usedCnt, capacity);
		tps = 0;
		Sleep(999);
	}

}
#endif

int main() {

	setlocale(LC_ALL, "");

	CObjectFreeList<stNode> freeList(false, false, 0);

	
#ifdef LOGIC_TEST
	logicTest();
#endif 

	return 0;
}