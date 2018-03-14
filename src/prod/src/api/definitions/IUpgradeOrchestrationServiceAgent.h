// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IUpgradeOrchestrationServiceAgent
    {
    public:
        virtual ~IUpgradeOrchestrationServiceAgent() {};

        virtual void Release() = 0;

        virtual void RegisterUpgradeOrchestrationService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            IUpgradeOrchestrationServicePtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterUpgradeOrchestrationService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;
    };
}
