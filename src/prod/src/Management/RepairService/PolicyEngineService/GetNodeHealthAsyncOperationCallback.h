// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class GetNodeHealthAsyncOperationCallback : public FabricAsyncOperationCallback
    {
    public:
        Common::ComPointer<IFabricNodeHealthResult> Result;

        void Initialize(Common::ComPointer<IFabricHealthClient> fabricHealthClientCPtr);
        HRESULT STDMETHODCALLTYPE Wait(void);

        /*IFabricAsyncOperationCallback members*/
        void STDMETHODCALLTYPE Invoke(/* [in] */ IFabricAsyncOperationContext *context);

    private:
        Common::ComPointer<IFabricHealthClient> _fabricHealthClientCPtrCPtr;
        Common::ManualResetEvent _completed;
        HRESULT _hresult;
    };
}
