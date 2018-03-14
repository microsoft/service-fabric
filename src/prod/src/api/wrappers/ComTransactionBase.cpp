// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComTransactionBase Implementation
//

ComTransactionBase::ComTransactionBase(ITransactionBasePtr const & impl)
    : IFabricTransactionBase(),
    ComUnknownBase(),
    impl_(impl),
    id_()
{
    id_ = impl_->get_Id().AsGUID();
}

ComTransactionBase::~ComTransactionBase()
{
}

const FABRIC_TRANSACTION_ID * ComTransactionBase::get_Id( void)
{
    return &id_;
}
        
FABRIC_TRANSACTION_ISOLATION_LEVEL ComTransactionBase::get_IsolationLevel( void)
{
    return impl_->get_IsolationLevel();
}

