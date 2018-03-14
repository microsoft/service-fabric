// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IBackupRestoreServiceAgentFactory
    {
    public:
        virtual ~IBackupRestoreServiceAgentFactory() {};

        virtual Common::ErrorCode CreateBackupRestoreServiceAgent(
            __out IBackupRestoreServiceAgentPtr &) = 0;
    };
}
