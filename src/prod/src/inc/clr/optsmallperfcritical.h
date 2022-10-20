// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef FPO_ON
#error Recursive use of FPO_ON not supported
#endif

#define FPO_ON 1


#if defined(_MSC_VER) && !defined(_DEBUG)
 #pragma optimize("t", on)   // optimize for speed
 #if !defined(_AMD64_)   // 'y' isn't an option on amd64
  #pragma optimize("y", on)   // omit frame pointer
 #endif // !defined(_TARGET_AMD64_)
#endif 
