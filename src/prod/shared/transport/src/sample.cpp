// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


// Sample.CPP
// Compiles for both user mode and kernel mode


#include <ktl.h>
#include <ikmaster.h>


#include <malloc.h>


void TestFunction()
{

    void* p = _malloca(128);


    int x = 33;

    // Do nothing
}

