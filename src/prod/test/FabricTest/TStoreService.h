// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    //
    // TStore based service
    //
    class TStoreService : public TXRService
    {
        DENY_COPY(TStoreService);

    public:
        static std::wstring const DefaultServiceType;

        TStoreService(
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::wstring const& serviceName,
            Federation::NodeId nodeId,
            std::wstring const& workingDir,
            TestCommon::TestSession & testSession,
            std::wstring const& initDataStr,
            std::wstring const& serviceType = TStoreService::DefaultServiceType,
            std::wstring const& appId = L"");

        virtual ~TStoreService();

        Common::ErrorCode Put(
            __int64 key,
            std::wstring const& value,
            FABRIC_SEQUENCE_NUMBER currentVersion,
            std::wstring serviceName,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            ULONGLONG & dataLossVersion) override;

        Common::ErrorCode Get(
            __int64 key,
            std::wstring serviceName,
            std::wstring & value,
            FABRIC_SEQUENCE_NUMBER & version,
            ULONGLONG & dataLossVersion) override;

        Common::ErrorCode GetAll(
            std::wstring serviceName,
            std::vector<std::pair<__int64,std::wstring>> & kvpairs) override;

        Common::ErrorCode GetKeys(
            std::wstring serviceName,
            __int64 first,
            __int64 last,
            std::vector<__int64> & kvpairs) override;

        Common::ErrorCode Delete(
            __int64,
            FABRIC_SEQUENCE_NUMBER,
            std::wstring,
            FABRIC_SEQUENCE_NUMBER &,
            ULONGLONG &) override;

    private:
        std::wstring GetStateProviderName();
    };

    typedef std::shared_ptr<TStoreService> TStoreServiceSPtr;
}
