// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator 
{
    // 
    // Represents the context passed in by the state provider as part of an operation in a transaction/atomic operation
    //
    class OperationContext 
        : public KObject<OperationContext>
        , public KShared<OperationContext>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(OperationContext)
    };
}
