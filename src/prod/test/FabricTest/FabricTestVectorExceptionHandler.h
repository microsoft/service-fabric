// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestVectorExceptionHandler
    {
    public:

        static void Register()
        {
#if !defined(PLATFORM_UNIX) 
            AddVectoredExceptionHandler(1, VectorExceptionHandler);
#endif            
        }

        static LONG WINAPI VectorExceptionHandler(PEXCEPTION_POINTERS pException)
        {
            if (pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
            {
                ::RaiseFailFastException(NULL, NULL, 1);
            }

            return EXCEPTION_CONTINUE_SEARCH;
        }

    };
}
