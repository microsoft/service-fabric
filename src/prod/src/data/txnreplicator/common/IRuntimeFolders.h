// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IRuntimeFolders
    {
        K_SHARED_INTERFACE(IRuntimeFolders);
        
    public:

        virtual LPCWSTR get_WorkDirectory() const = 0;
    };
}
