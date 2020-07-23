// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComGatewayResourceManagerAgentFactory
    {
    public:
        ComGatewayResourceManagerAgentFactory(IGatewayResourceManagerAgentFactoryPtr const & impl);
        virtual ~ComGatewayResourceManagerAgentFactory();

        /* [entry] */ HRESULT CreateFabricGatewayResourceManagerAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricGatewayResourceManagerAgent);

    private:
        IGatewayResourceManagerAgentFactoryPtr impl_;
    };
}

