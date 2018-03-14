// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

LockContext::LockContext()
    : OperationContext()
    , isTracking_(false)
{
}

LockContext::~LockContext()
{
}

bool LockContext::IsTrackingContext()
{
    return isTracking_;
}
