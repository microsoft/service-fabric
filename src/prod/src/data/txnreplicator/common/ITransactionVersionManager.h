// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface ITransactionVersionManager
    {
        K_SHARED_INTERFACE(ITransactionVersionManager)

    public:
        virtual ktl::Awaitable<NTSTATUS> RegisterAsync(__out LONG64 & versionNumber) noexcept = 0;

        virtual NTSTATUS UnRegister(LONG64 visibilityVersionNumber) noexcept = 0;
    };
}
