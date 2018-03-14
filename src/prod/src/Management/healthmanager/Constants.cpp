// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::HealthManager;

// -------------------------------
// General
// -------------------------------

GlobalWString Constants::TokenDelimeter = make_global<wstring>(L"+");
FABRIC_INSTANCE_ID Constants::UnusedInstanceValue = FABRIC_INVALID_INSTANCE_ID - 1;

// -------------------------------
// Store Data Keys
// -------------------------------

GlobalWString Constants::StoreType_ReplicaHealthEvent = make_global<wstring>(L"ReplicaHealthEvent");
GlobalWString Constants::StoreType_NodeHealthEvent = make_global<wstring>(L"NodeHealthEvent");
GlobalWString Constants::StoreType_PartitionHealthEvent = make_global<wstring>(L"PartitionHealthEvent");
GlobalWString Constants::StoreType_ServiceHealthEvent = make_global<wstring>(L"ServiceHealthEvent");
GlobalWString Constants::StoreType_ApplicationHealthEvent = make_global<wstring>(L"ApplicationHealthEvent");
GlobalWString Constants::StoreType_DeployedApplicationHealthEvent = make_global<wstring>(L"DeployedApplicationHealthEvent");
GlobalWString Constants::StoreType_DeployedServicePackageHealthEvent = make_global<wstring>(L"DeployedServicePackageHealthEvent");
GlobalWString Constants::StoreType_ClusterHealthEvent = make_global<wstring>(L"ClusterHealthEvent");

GlobalWString Constants::StoreType_ReplicaHealthAttributes = make_global<wstring>(L"ReplicaHealthAttributes");
GlobalWString Constants::StoreType_NodeHealthAttributes = make_global<wstring>(L"NodeHealthAttributes");
GlobalWString Constants::StoreType_PartitionHealthAttributes = make_global<wstring>(L"PartitionHealthAttributes");
GlobalWString Constants::StoreType_ServiceHealthAttributes = make_global<wstring>(L"ServiceHealthAttributes");
GlobalWString Constants::StoreType_ApplicationHealthAttributes = make_global<wstring>(L"ApplicationHealthAttributes");
GlobalWString Constants::StoreType_DeployedApplicationHealthAttributes = make_global<wstring>(L"DeployedApplicationHealthAttributes");
GlobalWString Constants::StoreType_DeployedServicePackageHealthAttributes = make_global<wstring>(L"DeployedServicePackageHealthAttributes");
GlobalWString Constants::StoreType_ClusterHealthAttributes = make_global<wstring>(L"ClusterHealthAttributes");

GlobalWString Constants::StoreType_ReplicaSequenceStream = make_global<wstring>(L"ReplicaSequenceStream");
GlobalWString Constants::StoreType_NodeSequenceStream = make_global<wstring>(L"NodeSequenceStream");
GlobalWString Constants::StoreType_PartitionSequenceStream = make_global<wstring>(L"PartitionSequenceStream");
GlobalWString Constants::StoreType_ServiceSequenceStream = make_global<wstring>(L"ServiceSequenceStream");
GlobalWString Constants::StoreType_ApplicationSequenceStream = make_global<wstring>(L"ApplicationSequenceStream");
GlobalWString Constants::StoreType_DeployedApplicationSequenceStream = make_global<wstring>(L"DeployedApplicationSequenceStream");
GlobalWString Constants::StoreType_DeployedServicePackageSequenceStream = make_global<wstring>(L"DeployedServicePackageSequenceStream");
GlobalWString Constants::StoreType_ClusterSequenceStream = make_global<wstring>(L"ClusterSequenceStream");

// -------------------------------
// Sequence stream identifiers
// Used to form unique keys for the sequence stream
// Do not tie the names to the implementation of wformatString(FABRIC_HEALTH_REPORT_KIND),
// as we need to guarantee that the store format does not change.
// -------------------------------
GlobalWString Constants::StoreType_ReplicaTypeString = make_global<std::wstring>(L"ReplicaEntity");
GlobalWString Constants::StoreType_NodeTypeString = make_global<std::wstring>(L"NodeEntity");
GlobalWString Constants::StoreType_PartitionTypeString = make_global<std::wstring>(L"PartitionEntity");
GlobalWString Constants::StoreType_ServiceTypeString = make_global<std::wstring>(L"ServiceEntity");
GlobalWString Constants::StoreType_ApplicationTypeString = make_global<std::wstring>(L"ApplicationEntity");
GlobalWString Constants::StoreType_DeployedApplicationTypeString = make_global<std::wstring>(L"DeployedApplicationEntity");
GlobalWString Constants::StoreType_DeployedServicePackageTypeString = make_global<std::wstring>(L"DeployedServicePackageEntity");
GlobalWString Constants::StoreType_ClusterTypeString = make_global<std::wstring>(L"ClusterEntity");

// -------------------------------
// Job Queue identifiers
// -------------------------------

Common::GlobalWString Constants::ProcessReportJobItemId = make_global<wstring>(L"ProcessReportJI");
Common::GlobalWString Constants::CleanupEntityJobItemId = make_global<wstring>(L"CleanupEntityJI");
Common::GlobalWString Constants::CleanupEntityExpiredTransientEventsJobItemId = make_global<wstring>(L"CleanupEntityExpiredTransientEventsJobItemId");
Common::GlobalWString Constants::DeleteEntityJobItemId = make_global<wstring>(L"DeleteEntityJI");
Common::GlobalWString Constants::ProcessSequenceStreamJobItemId = make_global<wstring>(L"SequenceStreamJI");
Common::GlobalWString Constants::CheckInMemoryEntityDataJobItemId = make_global<wstring>(L"CheckInMemoryEntityData");

// -------------------------------
// Test parameters
// -------------------------------
double Constants::HealthEntityCacheLoadStoreDataDelayInSeconds = 0;

