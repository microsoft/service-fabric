// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComBackupRestoreServiceAgentFactory
    {
    public:
        ComBackupRestoreServiceAgentFactory(IBackupRestoreServiceAgentFactoryPtr const & impl);
        virtual ~ComBackupRestoreServiceAgentFactory();

        /* [entry] */ HRESULT CreateFabricBackupRestoreServiceAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricBackupRestoreServiceAgent);

    private:
        IBackupRestoreServiceAgentFactoryPtr impl_;
    };
}

