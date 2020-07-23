// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

HRESULT STDMETHODCALLTYPE RepairAsyncOperationCallback::Wait(void)
{
    _completed.WaitOne();
    return _hresult;
}

void RepairAsyncOperationCallback::Initialize(Common::ComPointer<IFabricRepairManagementClient> fabricRepairManagementClientCPtr, 
                                              std::shared_ptr<NodeRepairInfo> nodeRepairInfo,
                                              EndFunction endFunction)
{
    Initialize(fabricRepairManagementClientCPtr);
    _nodeRepairInfo = nodeRepairInfo;
    _endFunction = endFunction;
}

void RepairAsyncOperationCallback::Initialize(Common::ComPointer<IFabricRepairManagementClient> fabricRepairManagementClientCPtr)
{
    _hresult = S_OK;
    _completed.Reset();
    _fabricRepairManagementClientCPtr = fabricRepairManagementClientCPtr;
    _endFunction = nullptr;
}

void RepairAsyncOperationCallback::Invoke(/* [in] */ IFabricAsyncOperationContext *context)
{
    // If the class is being used synchronously this can be null
    if(_endFunction)
    {
        _endFunction(context, _fabricRepairManagementClientCPtr, _nodeRepairInfo);
    }
    _completed.Set();
}
