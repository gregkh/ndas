/*
 -------------------------------------------------------------------------
 Copyright (C) 2011, IOCELL Networks Corp. Plainsboro, NJ, USA.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary 
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright 
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products 
      built using this software without specific written permission. 
      
 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.
 
 DISCLAIMER

 This software is provided 'as is' with no explcit or implied warranties
 in respect of any properties, including, but not limited to, correctness 
 and fitness for purpose.
 -------------------------------------------------------------------------
*/
#ifndef _SAL_SYS_GENERIC_SAL_H_
#define _SAL_SYS_GENERIC_SAL_H_

/* SAL API attribute modifier */
#if (__GNUC__ > 3 || __GNUC__ == 3 ) && defined(_X86)
#define NDAS_CALL __attribute__((regparm(3)))
#else
#define NDAS_CALL
#endif

#if defined(XPLAT_EMBEDDED)
#include "sal/linux/emb.h"
#endif  /* _MIPSEL */

#endif /* _SAL_SYS_GENERIC_SAL_H_ */
