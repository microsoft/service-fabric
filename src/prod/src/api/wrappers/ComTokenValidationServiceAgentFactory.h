// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComTokenValidationServiceAgentFactory
    {
    public:
        ComTokenValidationServiceAgentFactory(ITokenValidationServiceAgentFactoryPtr const & impl);
        virtual ~ComTokenValidationServiceAgentFactory();

        /* [entry] */ HRESULT CreateFabricTokenValidationServiceAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricTokenValidationServiceAgent);

    private:
        ITokenValidationServiceAgentFactoryPtr impl_;
    };
}

