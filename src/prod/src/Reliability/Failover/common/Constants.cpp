// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;
using std::wstring;

// Configuration

Common::StringLiteral const Constants::FTSource("FT");
Common::StringLiteral const Constants::FTUpdateFailureSource("FTUpdateFailure");

Common::StringLiteral const Constants::NodeSource("Node");
Common::StringLiteral const Constants::NodeUpSource("NodeUp");
Common::StringLiteral const Constants::NodeUpdateSource("NodeUpdate");
Common::StringLiteral const Constants::NodeCacheSource("NodeCache");
Common::StringLiteral const Constants::NodeStateSource("NodeState");

Common::StringLiteral const Constants::ServiceSource("Service");
Common::StringLiteral const Constants::ServiceTypeSource("ServiceType");
Common::StringLiteral const Constants::AppUpdateSource("AppUpdate");

Common::StringLiteral const Constants::QuerySource("Query");

Common::StringLiteral const Constants::AdminApiSource("AdminAPI");

Common::StringLiteral const Constants::ServiceResolverSource("ServiceResolver");

Common::StringLiteral const Constants::NIMApiSource("NIMApi");

int64 const Constants::InvalidConfigurationVersion = 0;
int64 const Constants::InvalidDataLossVersion = 0;
int64 const Constants::InvalidReplicaId = -1;

int64 const Constants::InvalidNodeActivationSequenceNumber = -1;

size_t const Constants::FMStoreBufferSize = 8192;
size_t const Constants::RAStoreBufferSize = 8192;

/*
    This constant is used by the RA to keep track of a replica that does not know its progress during Phase1_GetLSN.     
*/
FABRIC_SEQUENCE_NUMBER const Constants::UnknownLSN = -2;

GlobalWString Constants::IpcServerMonitoringListenerName = make_global<wstring>(L"Reliability.IpcServer");

GlobalWString Constants::InvalidPrimaryLocation = make_global<wstring>(L"InvalidPrimaryLocation");
GlobalWString Constants::NotApplicable = make_global<wstring>(L"NotApplicable");

GlobalWString Constants::LookupVersionKey = make_global<wstring>(L"LookupVersion");
GlobalWString Constants::GenerationKey = make_global<wstring>(L"Generation");

// TODO: update after finalize the upgrade domain issues.
Global<Uri> Constants::DefaultUpgradeDomain = make_global<Uri>(L"ud", L"default");

Common::Guid const Constants::FMServiceGuid(L"00000000-0000-0000-0000-000000000001");
ConsistencyUnitId const Constants::FMServiceConsistencyUnitId(Constants::FMServiceGuid);

GlobalWString Constants::TombstoneServiceType = make_global<wstring>(L"_TSType_");
GlobalWString Constants::TombstoneServiceName = make_global<wstring>(L"_TS_");

GlobalWString const Constants::FabricApplicationId = make_global<wstring>(L"{EA75A378-98ED-4E61-A6F7-EAFB80FAB890}");
GlobalWString const Constants::SystemServiceUriScheme = make_global<wstring>(L"sys");

GlobalWString const Constants::FMServiceResolverTag = make_global<wstring>(L"FM");
GlobalWString const Constants::FMMServiceResolverTag = make_global<wstring>(L"FMM");

GlobalWString const Constants::HealthReportReplicatorTag = make_global<wstring>(L"Replicator");
GlobalWString const Constants::HealthReportServiceTag = make_global<wstring>(L"Service");
GlobalWString const Constants::HealthReportOperationTag = make_global<wstring>(L"Operation");
GlobalWString const Constants::HealthReportOpenTag = make_global<wstring>(L"Open");
GlobalWString const Constants::HealthReportChangeRoleTag = make_global<wstring>(L"ChangeRole");
GlobalWString const Constants::HealthReportCloseTag = make_global<wstring>(L"Close");
GlobalWString const Constants::HealthReportAbortTag = make_global<wstring>(L"Abort");
GlobalWString const Constants::HealthReportDurationTag = make_global<wstring>(L"Duration");

