// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFaultAnalysisServiceAgentFactory
    {
    public:
        ComFaultAnalysisServiceAgentFactory(IFaultAnalysisServiceAgentFactoryPtr const & impl);
        virtual ~ComFaultAnalysisServiceAgentFactory();

        /* [entry] */ HRESULT CreateFabricFaultAnalysisServiceAgent(
            /* [in] */ __RPC__in REFIID riid,
            /* [out, retval] */ __RPC__deref_out_opt void ** fabricFaultAnalysisServiceAgent);

    private:
        IFaultAnalysisServiceAgentFactoryPtr impl_;
    };
}

