#ifdef _DEBUG
#define DEBUG_WAS_DEFINED 1
#undef _DEBUG
#endif

#include <Python.h>

#ifdef DEBUG_WAS_DEFINED
#define _DEBUG 1
#endif


