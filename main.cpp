#include <Windows.h>
#include <stdio.h>
#include <new>

#include "SimpleProfiler.h"
#include "ObjectFreeList.h"

#define LOGIC_TEST
//#define SPEED_TEST


class CTest {

public:
	int a;

	CTest() {
		a = 0;
		//wprintf(L"\taddr: 0x%x, Constructor Call\n", this);
	}

	~CTest() {
		//wprintf(L"\taddr: 0x%x, Destructor Call\n",this);
	}
};

#ifdef LOGIC_TEST
void logicTest() {

	constexpr int OBJECT_NUM = 10;

	CObjectFreeList<CTest> testFreeList(OBJECT_NUM);
	CTest* arr[OBJECT_NUM];

	// 기존에 할당받은 경우가 없을 때, 정상적으로 할당받을 수 있다.
	// 이 때 생성자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"ALLOC\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		arr[objectCnt] = testFreeList.alloc();
		arr[objectCnt]->a = objectCnt;
		wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
	}
	wprintf(L"---------------------------------\n");

	// 할당받은 경우, 해당 포인터로 반환할 수 있었다.
	// 이때 소멸자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
		testFreeList.free(arr[objectCnt]);
		arr[objectCnt] = nullptr;
	}
	wprintf(L"---------------------------------\n");

	// 내가 해제한 포인터들을 재할당받을 수 있었다.
	// 이 때 생성자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"ALLOC\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		arr[objectCnt] = testFreeList.alloc();
		wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
	}
	wprintf(L"---------------------------------\n");

	// 할당 받은 포인터들에 대해서 언더플로우 발생
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE UnderFlow\n");
	wprintf(L"---------------------------------\n");
	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		((CTest*)(((char*)arr[objectCnt]) - sizeof(void*)))->a = 123;
		
		{
			// free underflow
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				wprintf(L"underflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
		}

		{
			// 데이터 수정 후, 다시 해제 시도
			((CTest*)(((char*)arr[objectCnt]) - sizeof(void*)))->a = 0xf9f9f9f9f9f9f9f9;
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				wprintf(L"error: underflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
			if (freeResult == 1) {
				wprintf(L"error: overflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
		}
	}
	wprintf(L"---------------------------------\n");

	// 할당 받은 포인터들에 대해서 오버플로우 발생
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE OverFlow\n");
	wprintf(L"---------------------------------\n");
	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		((CTest*)(((char*)arr[objectCnt]) + sizeof(void*)))->a = 123;

		{
			// free overFlow
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == 1) {
				wprintf(L"overflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
		}

		{
			// 데이터 수정 후, 다시 해제 시도
			((CTest*)(((char*)arr[objectCnt]) + sizeof(void*)))->a = 0xf9f9f9f9f9f9f9f9;
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				wprintf(L"error: underflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
			if (freeResult == 1) {
				wprintf(L"error: overflow, addr: 0x%016I64x\n", arr[objectCnt]);
			}
		}
	}
	wprintf(L"---------------------------------\n");
	// freelist 객체가 소멸하면서 생성한 객체를 정리한다.
}
#endif

#ifdef SPEED_TEST
void speedTest() {
	SimpleProfiler sp;

	constexpr int TEST_NUM = 10000000;
	wprintf(L"sizeof: %d\n",sizeof(CTest));
	CObjectFreeList<CTest> testFreeList(TEST_NUM);
	CTest** arr = new CTest*[TEST_NUM];
	CTest** arrBegin = arr;
	CTest** arrEnd = arr + TEST_NUM;

	void* minPtr = (void*)0xFFFFFFFFFFFFFFFF;
	void* maxPtr = 0;

	for (CTest** arrIter = arrBegin; arrIter != arrEnd; ++arrIter) {
		sp.profileBegin("c++ new");
		*arrIter = new CTest();
		if (minPtr > *arrIter) {
			minPtr = *arrIter;
		}
		if (maxPtr < *arrIter) {
			maxPtr = *arrIter;
		}
		sp.profileEnd("c++ new");
	}

	wprintf(L"minPtr: 0x%016I64x, maxPtr: 0x%016I64x, distance: %I64d\n", (__int64)minPtr, (__int64)maxPtr, (__int64)maxPtr - (__int64)minPtr);

	for (CTest** arrIter = arrBegin; arrIter != arrEnd; ++arrIter) {
		sp.profileBegin("c++ delete");
		delete* arrIter;
		sp.profileEnd("c++ delete");
	}

	for (CTest** arrIter = arrBegin; arrIter != arrEnd; ++arrIter) {
		sp.profileBegin("ReAlloc");
		*arrIter = testFreeList.alloc();
		sp.profileEnd("ReAlloc");
	}

	wprintf(L"minPtr: 0x%016I64x, maxPtr: 0x%016I64x. distance: %I64d\n", (__int64)arr[0], (__int64)arr[TEST_NUM - 1], (__int64)arr[TEST_NUM-1] - (__int64)arr[0]);

	for (CTest** arrIter = arrBegin; arrIter != arrEnd; ++arrIter) {
		sp.profileBegin("Free");
		testFreeList.free(*arrIter);
		sp.profileEnd("Free");
	}

	sp.printToFile();

	delete[] arr;
}
#endif 

int main() {

#ifdef LOGIC_TEST
	logicTest();
#endif 


#ifdef SPEED_TEST
	speedTest();
#endif


	return 0;
}