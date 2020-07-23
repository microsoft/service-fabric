// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class GatewayResourceManagerAgentFactory
            : public Api::IGatewayResourceManagerAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~GatewayResourceManagerAgentFactory();

            static Api::IGatewayResourceManagerAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateGatewayResourceManagerAgent(Api::IGatewayResourceManagerAgentPtr &);

        private:
            GatewayResourceManagerAgentFactory();
        };
    }
}

