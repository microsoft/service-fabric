// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class Constants
    {

    public:

        // Trace Sources
        static Common::StringLiteral const FTSource;
        static Common::StringLiteral const FTUpdateFailureSource;

        static Common::StringLiteral const NodeSource;
        static Common::StringLiteral const NodeUpSource;
        static Common::StringLiteral const NodeUpdateSource;
        static Common::StringLiteral const NodeCacheSource;
        static Common::StringLiteral const NodeStateSource;

        static Common::StringLiteral const ServiceSource;
        static Common::StringLiteral const ServiceTypeSource;
        static Common::StringLiteral const AppUpdateSource;

        static Common::StringLiteral const QuerySource;

        static Common::StringLiteral const AdminApiSource;

        static Common::StringLiteral const ServiceResolverSource;

        static Common::StringLiteral const NIMApiSource;

        static int64 const InvalidConfigurationVersion;
        static int64 const InvalidDataLossVersion;
        static int64 const InvalidReplicaId;

        static size_t const FMStoreBufferSize;
        static size_t const RAStoreBufferSize;

        static int64 const InvalidNodeActivationSequenceNumber;

        static FABRIC_SEQUENCE_NUMBER const UnknownLSN;

        static Common::GlobalWString IpcServerMonitoringListenerName;

        static Common::GlobalWString InvalidPrimaryLocation;
        static Common::GlobalWString NotApplicable;

        static Common::GlobalWString LookupVersionKey;
        static Common::GlobalWString GenerationKey;
        
        static Common::Guid const FMServiceGuid;
        static ConsistencyUnitId const FMServiceConsistencyUnitId;
        static Common::Global<Common::Uri> DefaultUpgradeDomain;

        static Common::GlobalWString TombstoneServiceType;
        static Common::GlobalWString TombstoneServiceName;

        static Common::GlobalWString const FabricApplicationId;
        static Common::GlobalWString const SystemServiceUriScheme;
        static Common::GlobalWString const FMServiceResolverTag;
        static Common::GlobalWString const FMMServiceResolverTag;

        // Health Reporting 
        static Common::GlobalWString const HealthReportReplicatorTag;
        static Common::GlobalWString const HealthReportServiceTag;
        static Common::GlobalWString const HealthReportOperationTag;
        static Common::GlobalWString const HealthReportOpenTag;
        static Common::GlobalWString const HealthReportChangeRoleTag;
        static Common::GlobalWString const HealthReportCloseTag;
        static Common::GlobalWString const HealthReportAbortTag;
        static Common::GlobalWString const HealthReportDurationTag;
    };
};

