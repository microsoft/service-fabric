// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;

GlobalWString HealthAttributeNames::IpAddressOrFqdn = make_global<wstring>(L"IpAddressOrFqdn");
GlobalWString HealthAttributeNames::UpgradeDomain = make_global<wstring>(L"UpgradeDomain");
GlobalWString HealthAttributeNames::FaultDomain = make_global<wstring>(L"FaultDomain");

GlobalWString HealthAttributeNames::ApplicationName = make_global<wstring>(L"AppName");
GlobalWString HealthAttributeNames::ApplicationDefinitionKind = make_global<wstring>(L"ApplicationDefinitionKind");
GlobalWString HealthAttributeNames::ApplicationTypeName = make_global<wstring>(L"AppType");
GlobalWString HealthAttributeNames::ServiceTypeName = make_global<wstring>(L"SvcTypeName");
GlobalWString HealthAttributeNames::ServiceManifestName = make_global<wstring>(L"SvcManifestName");
GlobalWString HealthAttributeNames::ServicePackageActivationId = make_global<wstring>(L"ServicePackageActivationId");
GlobalWString HealthAttributeNames::ServiceName = make_global<wstring>(L"SvcName");

GlobalWString HealthAttributeNames::NodeId = make_global<wstring>(L"NodeId");
GlobalWString HealthAttributeNames::NodeName = make_global<wstring>(L"NodeName");
GlobalWString HealthAttributeNames::NodeInstanceId = make_global<wstring>(L"NodeInstanceId");
GlobalWString HealthAttributeNames::ProcessId = make_global<wstring>(L"PID");
GlobalWString HealthAttributeNames::ProcessName = make_global<wstring>(L"ProcName");

GlobalWString HealthAttributeNames::ApplicationHealthPolicy = make_global<wstring>(L"AppHealthPolicy");
GlobalWString HealthAttributeNames::ClusterHealthPolicy = make_global<wstring>(L"ClusterHealthPolicy");
GlobalWString HealthAttributeNames::ClusterUpgradeHealthPolicy = make_global<wstring>(L"ClusterUpgradeHealthPolicy");
GlobalWString HealthAttributeNames::ApplicationHealthPolicies = make_global<wstring>(L"ApplicationHealthPolicies");

