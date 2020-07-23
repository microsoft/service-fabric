// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../../ServiceModel/data/txnreplicator/SLConfigMacros.h"

namespace Management
{
    namespace CentralSecretService
    {
        class CentralSecretServiceConfig :
            public Common::ComponentConfig,
            public Reliability::ReplicationComponent::REConfigBase,
            public TxnReplicator::TRConfigBase,
            public TxnReplicator::SLConfigBase
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(CentralSecretServiceConfig, "CentralSecretService")

        public:
            bool static IsCentralSecretServiceEnabled();

            // The IsEnabled for CentralSecretService
            PUBLIC_CONFIG_ENTRY(bool, L"CentralSecretService", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
            
            // The EncryptionCertificateThumbprint for CentralSecretService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"CentralSecretService", EncryptionCertificateThumbprint, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The EncryptionCertificateStoreName for CentralSecretService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"CentralSecretService", EncryptionCertificateStoreName, L"My", Common::ConfigEntryUpgradePolicy::Static);

            // The TargetReplicaSetSize for CentralSecretService
            PUBLIC_CONFIG_ENTRY(int, L"CentralSecretService", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for CentralSecretService
            PUBLIC_CONFIG_ENTRY(int, L"CentralSecretService", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The ReplicaRestartWaitDuration for CentralSecretService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"CentralSecretService", ReplicaRestartWaitDuration, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Static);
            // The QuorumLossWaitDuration for CentralSecretService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"CentralSecretService", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);
            // The StandByReplicaKeepDuration for CentralSecretService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"CentralSecretService", StandByReplicaKeepDuration, Common::TimeSpan::FromMinutes(60 * 24 * 7), Common::ConfigEntryUpgradePolicy::Static);
            // The PlacementConstraints for CentralSecretService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"CentralSecretService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
            INTERNAL_CONFIG_ENTRY(int, L"CentralSecretService", CompactionThresholdInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum delay for internal retries when failures are encountered
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"CentralSecretService", MaxOperationRetryDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Uses TStore for persisted stateful storage when set to true
            INTERNAL_CONFIG_ENTRY(bool, L"CentralSecretService", EnableTStore, false, Common::ConfigEntryUpgradePolicy::NotAllowed);

            RE_INTERNAL_CONFIG_PROPERTIES(L"CentralSecretService/Replication", 50, 8192, 314572800, 16384, 314572800);
            TR_INTERNAL_CONFIG_PROPERTIES(L"CentralSecretService/TransactionalReplicator2");
            SL_CONFIG_PROPERTIES(L"CentralSecretService/SharedLog");
        };
    }
}
