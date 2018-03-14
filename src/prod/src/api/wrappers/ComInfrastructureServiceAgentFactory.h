// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComInfrastructureServiceAgentFactory
    {
    public:
        ComInfrastructureServiceAgentFactory(IInfrastructureServiceAgentFactoryPtr const & impl);
        virtual ~ComInfrastructureServiceAgentFactory();

        /* [entry] */ HRESULT CreateFabricInfrastructureServiceAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricInfrastructureServiceAgent);

    private:
        IInfrastructureServiceAgentFactoryPtr impl_;
    };
}

