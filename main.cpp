#include <stdio.h>
#include <new>

#include "ObjectFreeList.h"


class CTest {

public:
	int a;

	CTest() {
	}

	~CTest() {
	}
};

int main() {

	CObjectFreeList<CTest> freeList;

	CTest* a = freeList.alloc();
	a->a = 1;
	printf("%d ", a->a);

	CTest* b = freeList.alloc();
	b->a = 2;
	printf("%d ", b->a);

	CTest* c = freeList.alloc();
	c->a = 3;
	printf("%d ", c->a);

	printf("\n");
	freeList.free(a);
	freeList.free(b);
	freeList.free(c);

	CTest* a2 = freeList.alloc();
	CTest* b2 = freeList.alloc();
	CTest* c2 = freeList.alloc();

	printf("%d %d %d\n\n",a2->a, b2->a, c2->a);


	printf("%d %d\n", freeList.getCapacity(), freeList.getUsedCount());

	return 0;
}