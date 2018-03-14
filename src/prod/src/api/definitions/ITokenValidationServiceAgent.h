// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ITokenValidationServiceAgent
    {
    public:
        virtual ~ITokenValidationServiceAgent() {};
        
        virtual void Release() = 0;

        virtual void RegisterTokenValidationService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            ITokenValidationServicePtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterTokenValidationService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;
    };
}
