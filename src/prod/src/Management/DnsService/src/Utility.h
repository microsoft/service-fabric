// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    struct HandleSPtr
    {
        HANDLE InvalidValue;
        HANDLE Handle;

        void Cleanup()
        {
            if (Handle != InvalidValue)
            {
                CloseHandle(Handle);
                Handle = InvalidValue;
            }
        }

        HandleSPtr(__in HANDLE handle, __in_opt HANDLE invalidValue) :
            Handle(handle), InvalidValue(invalidValue)
        {}

        ~HandleSPtr()
        {
            Cleanup();
        }
    };

    void ExecuteCommandLine(
        __in std::string const & cmdline, 
        __out std::string& output, 
        __in ULONG timeoutInSec,
        __in IDnsTracer& tracer);
}
