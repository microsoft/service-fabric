// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __new__hpp
#define __new__hpp

struct NoThrow { int x; };
extern const NoThrow nothrow;

void * __cdecl operator new(size_t n, const NoThrow&);
void * __cdecl operator new[](size_t n, const NoThrow&);

#ifdef _DEBUG
void DisableThrowCheck();
#endif

#endif