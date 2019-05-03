// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IGatewayResourceManagerAgent
    {
    public:
        virtual ~IGatewayResourceManagerAgent() {};

        virtual void Release() = 0;

        virtual void RegisterGatewayResourceManager(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            IGatewayResourceManagerPtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterGatewayResourceManager(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;
    };
}
