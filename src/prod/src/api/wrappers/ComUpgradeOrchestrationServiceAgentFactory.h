// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComUpgradeOrchestrationServiceAgentFactory
    {
    public:
        ComUpgradeOrchestrationServiceAgentFactory(IUpgradeOrchestrationServiceAgentFactoryPtr const & impl);
        virtual ~ComUpgradeOrchestrationServiceAgentFactory();

        /* [entry] */ HRESULT CreateFabricUpgradeOrchestrationServiceAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricUpgradeOrchestrationServiceAgent);

    private:
        IUpgradeOrchestrationServiceAgentFactoryPtr impl_;
    };
}

