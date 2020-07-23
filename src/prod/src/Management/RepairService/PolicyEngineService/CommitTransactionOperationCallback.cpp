// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

HRESULT STDMETHODCALLTYPE CommitTransactionOperationCallback::Wait(void)
{
    _completed.WaitOne();
    return _hresult;
}

void CommitTransactionOperationCallback::Initialize(ComPointer<IFabricTransaction> transactionPtr)
{
    CODING_ERROR_ASSERT(transactionPtr.GetRawPointer() != nullptr);
    _completed.Reset();
    transactionPtr_ = transactionPtr;
    _hresult = S_OK;
    SecureZeroMemory(&Result, sizeof(Result));
}

void CommitTransactionOperationCallback::Invoke(/* [in] */ IFabricAsyncOperationContext *context)
{
    _hresult = transactionPtr_->EndCommit(context, &Result);
    _completed.Set();
}
