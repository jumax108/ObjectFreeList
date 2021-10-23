#include <stdio.h>
#include <new>

#include "ObjectFreeList.h"

#define LOGIC_TEST
//#define SPEED_TEST


class CTest {

public:
	int a;

	CTest() {
		wprintf(L"\taddr: 0x%x, Constructor Call\n", this);
	}

	~CTest() {
		wprintf(L"\taddr: 0x%x, Destructor Call\n",this);
	}
};

#ifdef LOGIC_TEST
void LogicTest() {

	constexpr int OBJECT_NUM = 10;

	CObjectFreeList<CTest> testFreeList;
	CTest* arr[OBJECT_NUM];

	// 기존에 할당받은 경우가 없을 때, 정상적으로 할당받을 수 있다.
	// 이 때 생성자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"ALLOC\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		arr[objectCnt] = testFreeList.alloc();
		arr[objectCnt]->a = objectCnt;
		wprintf(L"cnt: %d, addr: 0x%x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
	}
	wprintf(L"---------------------------------\n");

	// 할당받은 경우, 해당 포인터로 반환할 수 있었다.
	// 이때 소멸자가 호출되었다.
	wprintf(L"---------------------------------\n");
	wprintf(L"FREE\n");
	wprintf(L"---------------------------------\n");

	for (int objectCnt = 0; objectCnt < OBJECT_NUM; ++objectCnt) {
		wprintf(L"cnt: %d, addr: 0x%x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
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
		wprintf(L"cnt: %d, addr: 0x%x, value: %d\n", objectCnt, arr[objectCnt], arr[objectCnt]->a);
	}
	wprintf(L"---------------------------------\n");

	// freelist 객체가 소멸하면서 생성한 객체를 정리한다.
}
#endif


int main() {

#ifdef LOGIC_TEST
	LogicTest();
#endif 


	return 0;
}