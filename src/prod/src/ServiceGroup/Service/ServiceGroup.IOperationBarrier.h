// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

namespace ServiceGroup
{
    static const GUID IID_IOperationBarrier = {0xd6e0c58d,0x59a1,0x48f0,{0x84,0xb4,0x02,0x58,0xd0,0xc2,0x0a,0x3e}};
    struct IOperationBarrier : public IFabricOperation
    {
        virtual HRESULT STDMETHODCALLTYPE Barrier(void) = 0;
        virtual HRESULT STDMETHODCALLTYPE Cancel(void) = 0;
    };
}

