// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class FaultAnalysisServiceAgentFactory
            : public Api::IFaultAnalysisServiceAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~FaultAnalysisServiceAgentFactory();

            static Api::IFaultAnalysisServiceAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateFaultAnalysisServiceAgent(__out Api::IFaultAnalysisServiceAgentPtr &);

        private:
            FaultAnalysisServiceAgentFactory();
        };
    }
}

