// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IClientSettings
    {
    public:
        virtual ~IClientSettings() {};

        virtual Common::ErrorCode SetSecurity(
            Transport::SecuritySettings && securitySettings) = 0;

        virtual Common::ErrorCode SetKeepAlive(
            ULONG keepAliveIntervalInSeconds) = 0;

        virtual ServiceModel::FabricClientSettings GetSettings() = 0;

        virtual Common::ErrorCode SetSettings(
            ServiceModel::FabricClientSettings && fabricClientSettings) = 0;
    };
}
