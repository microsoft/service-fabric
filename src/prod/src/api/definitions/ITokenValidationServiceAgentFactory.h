// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ITokenValidationServiceAgentFactory
    {
    public:
        virtual ~ITokenValidationServiceAgentFactory() {};

        virtual Common::ErrorCode CreateTokenValidationServiceAgent(
            __out ITokenValidationServiceAgentPtr &) = 0;
    };
}
