
#include <functional>
#include <Windows.h>
#include <stdio.h>
#include <new>
#include <locale>
#include <thread>

#include "profilerTLS/headers/profilerTLS.h"
#pragma comment(lib, "lib/profilerTLS/profilerTLS")

#include "../headers/ObjectFreeListTLS.h"

#include "log/headers/log.h"
#pragma comment(lib,"lib/log/log")


//#define LOGIC_TEST
#define SPEED_TEST

CDump dump;
CLog logger;


constexpr int ALLOC_NUM_EACH_THREAD = 10000;
constexpr int THREAD_NUM = 3;
constexpr int MAX_ALLOC_NUM = ALLOC_NUM_EACH_THREAD * THREAD_NUM;

CProfilerTLS sp;

#ifdef LOGIC_TEST

void* const nodeStartValue = (void*)0x1122330044556677;
void* const nodeEndValue   = (void*)0x8899AABBCCDDEEFF;
struct stNode{

	void* nodeStart;
	__int64 data;
	void* nodeEnd;

	stNode(){
		nodeStart = nodeStartValue;
		data = 0;
		nodeEnd = nodeEndValue;
	}

};

CObjectFreeListTLS<stNode>* nodeFreeList;
CObjectFreeListTLS<stNode>* nodeFreeListForDebug;
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

	srand((unsigned int)arg);

	stNode** nodeArr = new stNode*[ALLOC_NUM_EACH_THREAD];

	HANDLE thread = *(HANDLE*)arg;

	while(1){

		// 1. 노드를 ALLOC_NUM_EACH_THREAD 만큼 할당받는다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){
			sp.begin("alloc");
			nodeArr[nodeCnt] = nodeFreeList->allocObjectTLS();
			sp.end("alloc");
		}
		Sleep(0);

		// 2. 노드의 모든 데이터가 초기값과 일치하는지 확인한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){
			stNode* node = nodeArr[nodeCnt];
		
			if(node->nodeStart != nodeStartValue){
				nodeFreeListForDebug = nodeFreeList;
				nodeFreeList = nullptr;
				CDump::crash();
			}

			if(node->nodeEnd != nodeEndValue){
				nodeFreeListForDebug = nodeFreeList;
				nodeFreeList = nullptr;
				CDump::crash();
			}

			if(node->data != 0){
				nodeFreeListForDebug = nodeFreeList;
				nodeFreeList = nullptr;
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
				nodeFreeListForDebug = nodeFreeList;
				nodeFreeList = nullptr;
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
				nodeFreeListForDebug = nodeFreeList;
				nodeFreeList = nullptr;
				CDump::crash();
			}

		}

		// 7. 노드를 ALLOC_NUM_EACH_THREAD 만큼 해제한다.
		for(int nodeCnt = 0; nodeCnt < ALLOC_NUM_EACH_THREAD; ++nodeCnt){

			stNode* node = nodeArr[nodeCnt];
			sp.begin("free");
			nodeFreeList->freeObjectTLS(node);
			sp.end("free");

		}
		
		// 8. capacity가 MAX 값을 넘어가지 않았는지 확인한다.
		unsigned int usedCnt = nodeFreeList->getUsedCount();
		unsigned int capacity = nodeFreeList->getCapacity();
		if(capacity > MAX_ALLOC_NUM){
			//nodeFreeListForDebug = nodeFreeList;
			//nodeFreeList = nullptr;
			//CDump::crash();
		}

		InterlockedIncrement(&tps);
	}

	return 0;

}

void logicTest() {


	HANDLE heap = HeapCreate(0, 0, 0);

	nodeFreeList = new CObjectFreeListTLS<stNode>(false, false);

	HANDLE thread[THREAD_NUM];
	for(int threadCnt = 0; threadCnt < THREAD_NUM; ++threadCnt){

		thread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, logicTestThreadFunc, (void*)&thread[threadCnt], 0, nullptr);

	}

	while(1){
	
		unsigned int usedCnt = nodeFreeList->getUsedCount();
		unsigned int capacity = nodeFreeList->getCapacity();
		wprintf(L"tps: %d, usedCnt: %d, capacity: %d\n", tps, usedCnt, capacity);
		tps = 0;
		sp.printToFile();
		Sleep(999);
	}

}
#endif

#ifdef SPEED_TEST

struct stNode{

	char data[1000];

};

CObjectFreeListTLS<stNode>* nodeFreeList;

constexpr int loopNum = 100000;

int loopCntMax=0;

unsigned __stdcall freeListSpeedTest(void* arg){

	stNode** arr = new stNode*[ALLOC_NUM_EACH_THREAD];
	

	for (int loopCnt = 0; loopCnt < loopNum; ++loopCnt) {

		for (int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt) {

			arr[cnt] = nodeFreeList->allocObject();

		}

		for (int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt) {

			nodeFreeList->freeObject(arr[cnt]);

		}

	}
	

	for(int loopCnt = 0 ; loopCnt < loopNum; ++loopCnt){
	
		sp.begin("alloc");
		for(int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt){

			arr[cnt] = nodeFreeList->allocObject();

		}
		sp.end("alloc");
		
		sp.begin("free");
		for(int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt){
		
			nodeFreeList->freeObject(arr[cnt]);
			
		}
		sp.end("free");

	}
	
	return 0;
}

unsigned __stdcall newDeleteSpeedTest(void* arg){

	stNode** arr = new stNode*[ALLOC_NUM_EACH_THREAD];

	for (int loopCnt = 0; loopCnt < loopNum; ++loopCnt) {

		for (int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt) {

			arr[cnt] = new stNode;

		}

		for (int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt) {

			delete arr[cnt];

		}

	}

	


	for(int loopCnt = 0 ; loopCnt < loopNum; ++loopCnt){
	
		sp.begin("new");
		for(int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt){

			arr[cnt] = new stNode;

		}
		sp.end("new");
			
		sp.begin("delete");
		for(int cnt = 0; cnt < ALLOC_NUM_EACH_THREAD; ++cnt){

			delete arr[cnt];

		}
		sp.end("delete");

	}

	return 0;

}

#endif

int main() {

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	setlocale(LC_ALL, "");
		
#ifdef SPEED_TEST

	HANDLE heap = HeapCreate(0,0,0);
	nodeFreeList = new CObjectFreeListTLS<stNode>(false, false);
	
	HANDLE freeListThread[THREAD_NUM];
	HANDLE newDeleteThread[THREAD_NUM];
		
	for(int threadCnt = 0; threadCnt < THREAD_NUM; ++ threadCnt){
		freeListThread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, freeListSpeedTest, nullptr, 0, nullptr);
	}
		
	WaitForMultipleObjects(THREAD_NUM, freeListThread, true, INFINITE);
	printf("Free List Done\n");
	
	/*
	for(int threadCnt = 0; threadCnt < THREAD_NUM; ++ threadCnt){
		newDeleteThread[threadCnt] = (HANDLE)_beginthreadex(nullptr, 0, newDeleteSpeedTest, nullptr, 0, nullptr);
	}

	WaitForMultipleObjects(THREAD_NUM, newDeleteThread, true, INFINITE);
	printf("New Delete Done\n");
	*/
	sp.printToFile();

#endif


#ifdef LOGIC_TEST
	logicTest();
#endif 

	return 0;
}