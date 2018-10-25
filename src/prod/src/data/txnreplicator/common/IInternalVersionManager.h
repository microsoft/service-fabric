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

    public: 
        // Initialize the VersionManager
        virtual void Initialize(__in IVersionProvider & versionProvider) = 0;

        // Clean up the states
        virtual void Reuse() = 0;
    };
}
