// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IGatewayResourceManagerAgentFactory
    {
    public:
        virtual ~IGatewayResourceManagerAgentFactory() {};

        virtual Common::ErrorCode CreateGatewayResourceManagerAgent(
            __out IGatewayResourceManagerAgentPtr &) = 0;
    };
}
