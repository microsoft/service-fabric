// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IBackupFolderInfo
    {
        K_SHARED_INTERFACE(IBackupFolderInfo);

    public:
        __declspec(property(get = get_LogPathList)) KArray<KString::CSPtr> const & LogPathList;
        virtual KArray<KString::CSPtr> const & get_LogPathList() const = 0;
    };
}
