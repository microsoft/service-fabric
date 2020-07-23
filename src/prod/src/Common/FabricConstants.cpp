// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

GlobalWString FabricConstants::WindowsFabricAllowedUsersGroupName = make_global<wstring>(L"ServiceFabricAllowedUsers");
GlobalWString FabricConstants::WindowsFabricAllowedUsersGroupComment = make_global<wstring>(L"Members of this group are allowed to communicate with Service Fabric. Service Fabric service host processes must run under an account that is a member of this group.");
GlobalWString FabricConstants::WindowsFabricAdministratorsGroupName = make_global<wstring>(L"ServiceFabricAdministrators");
GlobalWString FabricConstants::WindowsFabricAdministratorsGroupComment = make_global<wstring>(L"Members of this group are allowed to administer a Service Fabric cluster.");


GlobalWString FabricConstants::FabricRegistryKeyPath = make_global<wstring>(L"Software\\Microsoft\\Service Fabric");
GlobalWString FabricConstants::FabricEtcConfigPath = make_global<wstring>(L"/etc/servicefabric/");
GlobalWString FabricConstants::FabricRootRegKeyName = make_global<wstring>(L"FabricRoot");
GlobalWString FabricConstants::FabricBinRootRegKeyName = make_global<wstring>(L"FabricBinRoot");
GlobalWString FabricConstants::FabricCodePathRegKeyName = make_global<wstring>(L"FabricCodePath");
GlobalWString FabricConstants::FabricDataRootRegKeyName = make_global<wstring>(L"FabricDataRoot");
GlobalWString FabricConstants::FabricLogRootRegKeyName = make_global<wstring>(L"FabricLogRoot");
GlobalWString FabricConstants::EnableCircularTraceSessionRegKeyName = make_global<wstring>(L"EnableCircularTraceSession");
GlobalWString FabricConstants::EnableUnsupportedPreviewFeaturesRegKeyName = make_global<wstring>(L"EnableUnsupportedPreviewFeatures");
GlobalWString FabricConstants::IsSFVolumeDiskServiceEnabledRegKeyName = make_global<wstring>(L"IsSFVolumeDiskServiceEnabled");
GlobalWString FabricConstants::UseFabricInstallerSvcKeyName = make_global<wstring>(L"UseFabricInstallerSvc");
GlobalWString FabricConstants::FabricHostServicePathRegKeyName = make_global<wstring>(L"FabricHostServicePath");
GlobalWString FabricConstants::UpdaterServicePathRegKeyName = make_global<wstring>(L"UpdaterServicePath");
#if defined(PLATFORM_UNIX)
GlobalWString FabricConstants::SfInstalledMobyRegKeyName = make_global<wstring>(L"SfInstalledMoby");
#endif

GlobalWString FabricConstants::AppsFolderName = make_global<wstring>(L"_App");
// Current cluster manifest name
GlobalWString FabricConstants::CurrentClusterManifestFileName = make_global<wstring>(L"ClusterManifest.current.xml");

GlobalWString FabricConstants::InfrastructureManifestFileName = make_global<wstring>(L"InfrastructureManifest.xml");
GlobalWString FabricConstants::TargetInformationFileName = make_global<wstring>(L"TargetInformation.xml");
GlobalWString FabricConstants::FabricFolderName = make_global<wstring>(L"Fabric");
GlobalWString FabricConstants::FabricDataFolderName = make_global<wstring>(L"Fabric.Data");
GlobalWString FabricConstants::FabricGatewayName = make_global<wstring>(L"FabricGateway.exe");;
GlobalWString FabricConstants::FabricApplicationGatewayName = make_global<wstring>(L"FabricApplicationGateway.exe");;

// Default Store locations
GlobalWString FabricConstants::DefaultFileConfigStoreLocation = make_global<wstring>(L"FabricConfig.cfg");
GlobalWString FabricConstants::DefaultPackageConfigStoreLocation = make_global<wstring>(L"Fabric.Package.Current.xml");
GlobalWString FabricConstants::DefaultSettingsConfigStoreLocation = make_global<wstring>(L"FabricHostSettings.xml");
GlobalWString FabricConstants::DefaultClientSettingsConfigStoreLocation = make_global<wstring>(L"FabricClientSettings.xml");

// Async File Read Constants
DWORD FabricConstants::MaxFileSize = 1073741824;

// DnsService related constants
GlobalWString FabricConstants::FabricDnsServerIPAddressRegKeyName = make_global<wstring>(L"FabricDnsServerIPAddress");

// Isolated network constants
GlobalWString FabricConstants::FabricIsolatedNetworkInterfaceRegKeyName = make_global<wstring>(L"FabricIsolatedNetworkInterfaceName");