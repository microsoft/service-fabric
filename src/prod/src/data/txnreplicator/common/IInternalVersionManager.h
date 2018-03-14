// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // Interface used to forward calls to Version Manager from LoggingReplicator.
    interface IInternalVersionManager
        : public ITransactionVersionManager
        , public ILoggingReplicatorToVersionManager
        , public IVersionManager
    {
        K_SHARED_INTERFACE(IInternalVersionManager)
    };
}
