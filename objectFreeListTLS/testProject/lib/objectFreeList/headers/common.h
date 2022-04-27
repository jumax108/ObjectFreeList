#pragma once

//#define OBJECT_FREE_LIST_DEBUG
#define USE_OWN_HEAP

namespace objectFreeList {

	// 데이터 포인터(stNode->data)에 이 값을 더하면 노드 포인터(stNode)가 된다 !
	#if defined(OBJECT_FREE_LIST_DEBUG)
		constexpr __int64 DATA_TO_NODE_PTR = -8;
	#else
		constexpr __int64 DATA_TO_NODE_PTR = 0;
	#endif
};