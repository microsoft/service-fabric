// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class GetNodeListAsyncOperationCallback : public FabricAsyncOperationCallback
    {

    public:
        Common::ComPointer<IFabricGetNodeListResult> Result;

        void Initialize(Common::ComPointer<IFabricQueryClient> fabricQueryClientCPtr);
        HRESULT STDMETHODCALLTYPE Wait(void);

        /*IFabricAsyncOperationCallback members*/
        void STDMETHODCALLTYPE Invoke(/* [in] */ IFabricAsyncOperationContext *context);

    private:
        Common::ComPointer<IFabricQueryClient> _fabricQueryClientCPtrCPtr;
        Common::ManualResetEvent _completed;
        HRESULT _hresult;
    };
}
