// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInfrastructureServiceAgentFactory
    {
    public:
        virtual ~IInfrastructureServiceAgentFactory() {};

        virtual Common::ErrorCode CreateInfrastructureServiceAgent(
            __out IInfrastructureServiceAgentPtr &) = 0;
    };
}
