// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IFaultAnalysisServiceAgent
    {
    public:
        virtual ~IFaultAnalysisServiceAgent() {};

        virtual void Release() = 0;

        virtual void RegisterFaultAnalysisService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            IFaultAnalysisServicePtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterFaultAnalysisService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;
    };
}
