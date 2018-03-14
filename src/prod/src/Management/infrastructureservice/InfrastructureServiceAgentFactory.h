// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace InfrastructureService
    {
        class InfrastructureServiceAgentFactory
            : public Api::IInfrastructureServiceAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~InfrastructureServiceAgentFactory();

            static Api::IInfrastructureServiceAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateInfrastructureServiceAgent(__out Api::IInfrastructureServiceAgentPtr &);

        private:
            InfrastructureServiceAgentFactory();
        };
    }
}

