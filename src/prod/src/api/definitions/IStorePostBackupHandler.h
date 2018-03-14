// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStorePostBackupHandler
    {
    public:
        virtual ~IStorePostBackupHandler() {};

        virtual Common::AsyncOperationSPtr BeginPostBackup(
            __in FABRIC_STORE_BACKUP_INFO const & info,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndPostBackup(
            __in Common::AsyncOperationSPtr const & operation, 
            __out bool & status) = 0;
    };
}
