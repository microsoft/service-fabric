// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class CommitTransactionOperationCallback : public FabricAsyncOperationCallback
    {
    public:
        FABRIC_SEQUENCE_NUMBER Result;

        void Initialize(Common::ComPointer<IFabricTransaction> transactionPtr);
        HRESULT STDMETHODCALLTYPE Wait(void);

        /*IFabricAsyncOperationCallback members*/
        void STDMETHODCALLTYPE Invoke(/* [in] */ IFabricAsyncOperationContext *context);

    private:
        Common::ComPointer<IFabricTransaction> transactionPtr_;
        Common::ManualResetEvent _completed;
        HRESULT _hresult;
    };
}
