#pragma once

//#define OBJECT_FREE_LIST_DEBUG

namespace nsObjectFreeList {

	#if defined(OBJECT_FREE_LIST_DEBUG)
		constexpr unsigned __int64 DATA_PTR_TO_NODE_PTR = -8;
	#else 
		constexpr unsigned __int64 DATA_PTR_TO_NODE_PTR = 0;
	#endif

}