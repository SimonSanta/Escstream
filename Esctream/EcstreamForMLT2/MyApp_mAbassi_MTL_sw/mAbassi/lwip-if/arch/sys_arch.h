#include "lwipopts.h"

#if defined(__ABASSI_H__) || defined(__MABASSI_H__) || defined(__UABASSI_H__)
  #ifdef OS_CMSIS_RTOS
	  #include "../CMSIS/sys_arch.h"
  #else
	  #include "../Abassi/sys_arch.h"
  #endif
#endif
