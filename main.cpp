#include <Windows.h>
#include <stdio.h>
#include <new>
#include <locale>

#include "SimpleProfiler.h"
#include "ObjectFreeList.h"

#define LOGIC_TEST
//#define SPEED_TEST


class CTest {

public:
	int a;

	CTest() {
		a = 0;
		#if defined(LOGIC_TEST)
			#ifdef _WIN64
				wprintf(L"\taddr: 0x%016I64x, Constructor Call\n", (unsigned __int64)this);
			#else
				wprintf(L"\taddr: 0x%08x, Constructor Call\n", (unsigned int)this);
			#endif
		#endif
	}

	__declspec(noinline) ~CTest() {
		#if defined(LOGIC_TEST)
			#ifdef _WIN64
				wprintf(L"\taddr: 0x%016I64x, Destructor Call\n", (unsigned __int64)this);
			#else
				wprintf(L"\taddr: 0x%08x, Destructor Call\n", (unsigned int)this);
			#endif
		#endif
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
		#ifdef _WIN64
			wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, (unsigned __int64)arr[objectCnt], arr[objectCnt]->a);
		#else
			wprintf(L"cnt: %d, addr: 0x%08x, value: %d\n", objectCnt, (unsigned int)arr[objectCnt], arr[objectCnt]->a);
		#endif
	}
	wprintf(L"---------------------------------\n");

	// 할당받은 경우, 해당 포인터로 반환할 수 있었다.
	// 이때 소멸자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		#ifdef _WIN64
			wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, (unsigned __int64)arr[objectCnt], arr[objectCnt]->a);
		#else
			wprintf(L"cnt: %d, addr: 0x%08x, value: %d\n", objectCnt, (unsigned int)arr[objectCnt], arr[objectCnt]->a);
		#endif
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
		#ifdef _WIN64
			wprintf(L"cnt: %d, addr: 0x%016I64x, value: %d\n", objectCnt, (unsigned __int64)arr[objectCnt], arr[objectCnt]->a);
		#else
			wprintf(L"cnt: %d, addr: 0x%08x, value: %d\n", objectCnt, (unsigned int)arr[objectCnt], arr[objectCnt]->a);
		#endif
	}
	wprintf(L"---------------------------------\n");

	// 할당 받은 포인터들에 대해서 언더플로우 발생
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE UnderFlow\n");
	wprintf(L"---------------------------------\n");
	for (int objectCnt = 0; objectCnt < OBJECT_NUM / 2; ++objectCnt) {
		((CTest*)(((char*)arr[objectCnt]) - sizeof(void*)))->a = 123;
		
		{
			// free underflow
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				#ifdef _WIN64
					wprintf(L"underflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"underflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
		}

		{
			// 데이터 수정 후, 다시 해제 시도
			((CTest*)(((char*)arr[objectCnt]) - sizeof(void*)))->a = 0xf9f9f9f9f9f9f9f9;
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				#ifdef _WIN64
					wprintf(L"error: underflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"error: underflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
			if (freeResult == 1) {
				#ifdef _WIN64
					wprintf(L"error: overflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"error: overflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
		}
	}
	wprintf(L"---------------------------------\n");

	// 할당 받은 포인터들에 대해서 오버플로우 발생
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE OverFlow\n");
	wprintf(L"---------------------------------\n");
	for (int objectCnt = OBJECT_NUM / 2; objectCnt < OBJECT_NUM; ++objectCnt) {
		((CTest*)(((char*)arr[objectCnt]) + sizeof(void*)))->a = 123;

		{
			// free overFlow
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == 1) {
				#ifdef _WIN64
					wprintf(L"overflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"overflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
		}

		{
			// 데이터 수정 후, 다시 해제 시도
			((CTest*)(((char*)arr[objectCnt]) + sizeof(void*)))->a = 0xf9f9f9f9f9f9f9f9;
			int freeResult = testFreeList.free(arr[objectCnt]);
			if (freeResult == -1) {
				#ifdef _WIN64
					wprintf(L"error: underflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"error: underflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
			if (freeResult == 1) {
				#ifdef _WIN64
					wprintf(L"error: overflow, addr: 0x%016I64x\n", (unsigned __int64)arr[objectCnt]);
				#else
					wprintf(L"error: overflow, addr: 0x%08x\n", (unsigned int)arr[objectCnt]);
				#endif
			}
		}
	}
	wprintf(L"---------------------------------\n");


	// 중복 free 시 예외처리
	
	wprintf(L"---------------------------------\n");
	wprintf(L"중복 Free\n");
	wprintf(L"---------------------------------\n");

	for (int arrCnt = 0; arrCnt < OBJECT_NUM; ++arrCnt) {

		arr[arrCnt] = testFreeList.alloc();
	}

	for (int arrCnt = 0; arrCnt < OBJECT_NUM; ++arrCnt) {
		int freeResult = testFreeList.free(arr[arrCnt]);
		freeResult = testFreeList.free(arr[arrCnt]);
		if (freeResult == -2) {
			#ifdef _WIN64
				wprintf(L"중복 free, addr: 0x%016I64x\n", (unsigned __int64)arr[arrCnt]);
			#else
				wprintf(L"중복 free, addr: 0x%08x\n", (unsigned int)arr[arrCnt]);
			#endif
		}
		else {
			wprintf(L"error: freeResult: %d\n", freeResult);
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

	#ifdef _WIN64
		wprintf(L"minPtr: 0x%016I64x, maxPtr: 0x%016I64x, distance: %I64d\n", (unsigned __int64)minPtr, (unsigned __int64)maxPtr, (unsigned __int64)maxPtr - (unsigned __int64)minPtr);
	#else
		wprintf(L"minPtr: 0x%08x, maxPtr: 0x%08x, distance: %d\n", (unsigned int)minPtr, (unsigned int)maxPtr, (unsigned int)maxPtr - (unsigned int)minPtr);
	#endif

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

	#ifdef _WIN64
		wprintf(L"minPtr: 0x%016I64x, maxPtr: 0x%016I64x. distance: %I64d\n", (unsigned __int64)arr[0], (unsigned __int64)arr[TEST_NUM - 1], (unsigned __int64)arr[TEST_NUM - 1] - (unsigned __int64)arr[0]);
	#else
		wprintf(L"minPtr: 0x%08x, maxPtr: 0x%08x. distance: %d\n", (unsigned int)arr[0], (unsigned int)arr[TEST_NUM - 1], (unsigned int)arr[TEST_NUM - 1] - (unsigned int)arr[0]);
	#endif

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

	setlocale(LC_ALL, "");

#ifdef LOGIC_TEST
	logicTest();
#endif 


#ifdef SPEED_TEST
	speedTest();
#endif


	return 0;
}