// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface ILoggingReplicatorToVersionManager
    {
        K_SHARED_INTERFACE(ILoggingReplicatorToVersionManager)

    public:

        virtual ~ILoggingReplicatorToVersionManager() {}

        virtual void UpdateDispatchingBarrierTask(__in CompletionTask & barrierTask) = 0;
    };
}
