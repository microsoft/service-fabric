// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

wstring const FabricTest::FabricTestConstants::TestNodesDirectory = L"ScaleMin";
wstring const FabricTest::FabricTestConstants::TestImageStoreDirectory = L"ImageStore";
wstring const FabricTest::FabricTestConstants::TestTracesDirectory = L"Traces";
wstring const FabricTest::FabricTestConstants::TestHostTracesDirectory = L"HostTraces";
wstring const FabricTest::FabricTestConstants::FMDataDirectory = L"FMShare";
wstring const FabricTest::FabricTestConstants::TestDropDirectory = L"FabricTestDrop";
wstring const FabricTest::FabricTestConstants::TestDataRootDirectory = L"DataRoot";
wstring const FabricTest::FabricTestConstants::TestBackupDirectory = L"Backup";

wstring const FabricTest::FabricTestConstants::SupportedServiceTypesSection(L"SupportedServiceTypes");
wstring const FabricTest::FabricTestConstants::CodePackageRunAsSection(L"RunAsUser");
wstring const FabricTest::FabricTestConstants::DefaultRunAsSection(L"DefaultRunAs");
wstring const FabricTest::FabricTestConstants::PackageSharingSection(L"PackageSharing");


// Service Group
wstring const FabricTest::FabricTestConstants::MemberServiceNameDelimiter(L"#");
wstring const FabricTest::FabricTestConstants::ServiceAddressDelimiter(L"%");
wstring const FabricTest::FabricTestConstants::ServiceAddressDoubleDelimiter(L"%%");
wstring const FabricTest::FabricTestConstants::ServiceAddressEscapedDelimiter(L"(%)");

//Predeployment constants
wstring const FabricTest::FabricTestConstants::ApplicationTypeName(L"ApplicationTypeName");
wstring const FabricTest::FabricTestConstants::ApplicationTypeVersion(L"ApplicationTypeVersion");
wstring const FabricTest::FabricTestConstants::ServiceManifestName(L"ServiceManifestName");
wstring const FabricTest::FabricTestConstants::NodeId(L"NodeId");
wstring const FabricTest::FabricTestConstants::SharingScope(L"SharingScope");
wstring const FabricTest::FabricTestConstants::SharedPackageNames(L"SharedPackageNames");
wstring const FabricTest::FabricTestConstants::VerifyInCache(L"VerifyInCache");
wstring const FabricTest::FabricTestConstants::VerifyShared(L"VerifyShared");

wstring const FabricTest::FabricTestConstants::NodeIdList(L"NodeIdList");
wstring const FabricTest::FabricTestConstants::ExpectedCount(L"ExpectedCount");
wstring const FabricTest::FabricTestConstants::ApplicationName(L"ApplicationName");
wstring const FabricTest::FabricTestConstants::ServicePackageActivationId(L"ServicePackageActivationId");

