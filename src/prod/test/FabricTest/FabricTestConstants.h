// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestConstants
    {
    public:
        static std::wstring const TestNodesDirectory;
        static std::wstring const TestImageStoreDirectory;
        static std::wstring const TestTracesDirectory;
        static std::wstring const TestHostTracesDirectory;
        static std::wstring const FMDataDirectory;
        static std::wstring const TestDropDirectory;
        static std::wstring const TestDataRootDirectory;
		static std::wstring const TestBackupDirectory;

        static std::wstring const SupportedServiceTypesSection;
        static std::wstring const CodePackageRunAsSection;
        static std::wstring const DefaultRunAsSection;
        static std::wstring const PackageSharingSection;
        // Service Group
        static std::wstring const MemberServiceNameDelimiter;
        static std::wstring const ServiceAddressDelimiter;
        static std::wstring const ServiceAddressDoubleDelimiter;
        static std::wstring const ServiceAddressEscapedDelimiter;

        //Predeployment constants
        static std::wstring const ApplicationTypeName;
        static std::wstring const ApplicationTypeVersion;
        static std::wstring const ServiceManifestName;
        static std::wstring const NodeId;
        static std::wstring const SharingScope;
        static std::wstring const SharedPackageNames;
        static std::wstring const VerifyInCache;
        static std::wstring const VerifyShared;

        static std::wstring const NodeIdList;
        static std::wstring const ExpectedCount;
        static std::wstring const ApplicationName;
        static std::wstring const ServicePackageActivationId;
    };
};
