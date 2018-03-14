// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class TokenValidationServiceAgentFactory
            : public Api::ITokenValidationServiceAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~TokenValidationServiceAgentFactory();

            static Api::ITokenValidationServiceAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateTokenValidationServiceAgent(Api::ITokenValidationServiceAgentPtr &);

        private:
            TokenValidationServiceAgentFactory();
        };
    }
}

