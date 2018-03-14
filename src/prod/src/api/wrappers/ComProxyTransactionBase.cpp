// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyTransactionBase Implementation
//
ComProxyTransactionBase::ComProxyTransactionBase(
    ComPointer<IFabricTransactionBase> const & comImpl)
    : ComponentRoot(),
    ITransactionBase(),
    comImpl_(comImpl)
{
}

ComProxyTransactionBase::~ComProxyTransactionBase()
{
}

Guid ComProxyTransactionBase::get_Id()
{
    return Guid(*(comImpl_->get_Id()));
}

FABRIC_TRANSACTION_ISOLATION_LEVEL ComProxyTransactionBase::get_IsolationLevel()
{
    return comImpl_->get_IsolationLevel();
}
