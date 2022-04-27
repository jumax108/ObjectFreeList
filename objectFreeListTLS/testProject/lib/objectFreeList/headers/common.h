#pragma once

//#define OBJECT_FREE_LIST_DEBUG
#define USE_OWN_HEAP

namespace objectFreeList {

	// ������ ������(stNode->data)�� �� ���� ���ϸ� ��� ������(stNode)�� �ȴ� !
	#if defined(OBJECT_FREE_LIST_DEBUG)
		constexpr __int64 DATA_TO_NODE_PTR = -8;
	#else
		constexpr __int64 DATA_TO_NODE_PTR = 0;
	#endif
};