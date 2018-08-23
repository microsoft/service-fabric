// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;

// -------------------------------
// General
// -------------------------------

GlobalWString Constants::TokenDelimeter = make_global<wstring>(L"+");
GlobalWString Constants::QualifiedTypeDelimeter = make_global<wstring>(L":");

uint64 const Constants::InitialApplicationVersionInstance(1);

// -------------------------------
// Service Description
// -------------------------------

GlobalWString Constants::ClusterManagerServicePrimaryCountName = make_global<wstring>(L"__ClusterManagerServicePrimaryCount__");
GlobalWString Constants::ClusterManagerServiceReplicaCountName = make_global<wstring>(L"__ClusterManagerServiceReplicaCount__");

// -------------------------------
// Store Data Keys
// -------------------------------

GlobalWString Constants::DatabaseDirectory = make_global<wstring>(L"CM");
GlobalWString Constants::DatabaseFilename = make_global<wstring>(L"CM.edb");
GlobalWString Constants::SharedLogFilename = make_global<wstring>(L"cmshared.log");

GlobalWString Constants::StoreType_GlobalApplicationInstanceCounter = make_global<wstring>(L"GlobalApplicationInstanceCounter");
GlobalWString Constants::StoreKey_GlobalApplicationInstanceCounter = make_global<wstring>(L"GlobalApplicationInstanceCounter");

GlobalWString Constants::StoreType_VerifiedFabricUpgradeDomains = make_global<wstring>(L"VerifiedFabricUpgradeDomains");
GlobalWString Constants::StoreKey_VerifiedFabricUpgradeDomains = make_global<wstring>(L"VerifiedFabricUpgradeDomains");

GlobalWString Constants::StoreType_FabricProvisionContext = make_global<wstring>(L"FabricProvisionContext");
GlobalWString Constants::StoreKey_FabricProvisionContext = make_global<wstring>(L"FabricProvisionContext");

GlobalWString Constants::StoreType_FabricUpgradeContext = make_global<wstring>(L"FabricUpgradeContext");
GlobalWString Constants::StoreKey_FabricUpgradeContext = make_global<wstring>(L"FabricUpgradeContext");

GlobalWString Constants::StoreType_FabricUpgradeStateSnapshot = make_global<wstring>(L"FabricUpgradeStateSnapshot");
GlobalWString Constants::StoreKey_FabricUpgradeStateSnapshot = make_global<wstring>(L"FabricUpgradeStateSnapshot");

GlobalWString Constants::StoreType_ApplicationTypeContext = make_global<wstring>(L"ApplicationTypeContext");
GlobalWString Constants::StoreType_ApplicationContext = make_global<wstring>(L"ApplicationContext");
GlobalWString Constants::StoreType_ApplicationUpgradeContext = make_global<wstring>(L"ApplicationUpgradeContext");
GlobalWString Constants::StoreType_GoalStateApplicationUpgradeContext = make_global<wstring>(L"GoalStateApplicationUpgradeContext");
GlobalWString Constants::StoreType_ComposeDeploymentContext = make_global<wstring>(L"ComposeDeploymentContext");
GlobalWString Constants::StoreType_SingleInstanceDeploymentContext = make_global<wstring>(L"SingleInstanceDeploymentContext");
GlobalWString Constants::StoreType_ServiceContext = make_global<wstring>(L"ServiceContext");
GlobalWString Constants::StoreType_InfrastructureTaskContext = make_global<wstring>(L"InfrastructureTaskContext");
GlobalWString Constants::StoreType_ApplicationUpdateContext = make_global<wstring>(L"ApplicationUpdateContext");
GlobalWString Constants::StoreType_ComposeDeploymentUpgradeContext = make_global<wstring>(L"ComposeDeploymentUpgradeContext");
GlobalWString Constants::StoreType_SingleInstanceDeploymentUpgradeContext = make_global<wstring>(L"SingleInstanceDeploymentUpgradeContext");

GlobalWString Constants::StoreType_ApplicationIdMap = make_global<wstring>(L"ApplicationIdMap");
GlobalWString Constants::StoreType_ServicePackage = make_global<wstring>(L"ServicePackage");
GlobalWString Constants::StoreType_ServiceTemplate = make_global<wstring>(L"ServiceTemplate");
GlobalWString Constants::StoreType_VerifiedUpgradeDomains = make_global<wstring>(L"VerifiedUpgradeDomains");

GlobalWString Constants::StoreType_ApplicationManifest = make_global<wstring>(L"ApplicationManifest");
GlobalWString Constants::StoreType_ServiceManifest = make_global<wstring>(L"ServiceManifests");
GlobalWString Constants::StoreType_ComposeDeploymentInstance = make_global<wstring>(L"ComposeDeploymentInstance");
GlobalWString Constants::StoreType_ContainerGroupDeploymentInstance = make_global<wstring>(L"ContainerGroupDeploymentInstance");
GlobalWString Constants::StoreType_SingleInstanceApplicationInstance = make_global<wstring>(L"SingleInstanceApplicationInstance");

GlobalWString Constants::StoreType_ComposeDeploymentInstanceCounter = make_global<wstring>(L"ComposeDeploymentInstanceCounter");
GlobalWString Constants::StoreKey_ComposeDeploymentInstanceCounter = make_global<wstring>(L"ComposeDeploymentInstanceCounter");

GlobalWString Constants::StoreType_Volume = make_global<wstring>(L"Volume");
// -------------------------------
// Naming Data Keys
// -------------------------------

GlobalWString Constants::ApplicationPropertyName = make_global<wstring>(L"ApplicationFlag");
GlobalWString Constants::ComposeDeploymentTypePrefix = make_global<wstring>(L"Compose_");
GlobalWString Constants::SingleInstanceDeploymentTypePrefix = make_global<wstring>(L"SingleInstance_");

// -------------------------------
// Test parameters
// -------------------------------
bool Constants::ReportHealthEntityApplicationType = true;
