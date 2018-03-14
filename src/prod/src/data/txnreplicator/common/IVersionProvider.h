// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IVersionProvider
    {
        K_SHARED_INTERFACE(IVersionProvider)
        K_WEAKREF_INTERFACE(IVersionProvider)

    public:
        virtual NTSTATUS GetVersion(__out LONG64 & version) const noexcept = 0;
    };
}
