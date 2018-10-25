// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ClearContainerHelper
    {
        class ClearContainerLogHelperConstants
        {
            public:
                static std::wstring const FabricApplicationNamePrefix;
                static std::wstring const ApplicationNameSuffix;
                static std::wstring const CodePackageInstanceIdFileName;
                static std::wstring const ContainerLogFileName;
                static std::wstring const CodePackageDirectoryName;
                static char const PathSeparator;
                static char const PathSeparatorReplacer;
                static string const DnsServiceIP4Address;
                static string const NDotsConfig;
        };

        // using container description. Preferred method
        std::wstring GetSimpleApplicationName(Hosting2::ContainerDescription const & containerDescription);

        std::wstring GetDigestedApplicationName(Hosting2::ContainerDescription const & containerDescription);

        std::wstring GetSandboxLogDirectory(Hosting2::ContainerDescription const & containerDescription);

        std::wstring GetLogMountDirectory(Hosting2::ContainerDescription const & containerDescription);

        uint64 GetNewContainerInstanceIdAtomic(Hosting2::ContainerDescription const & containerDescription, Common::TimeSpan const timeout);

        std::wstring GetContainerRelativeLogPath(Hosting2::ContainerDescription const & containerDescription, uint64 containerInstanceId);

        std::wstring GetContainerFullLogPath(Hosting2::ContainerDescription const & containerDescription, uint64 containerInstanceId);

        // using raw values.
        std::wstring GetSimpleApplicationName(std::wstring applicationName);

        std::wstring GetDigestedApplicationName(std::wstring applicationName);

        std::wstring GetSandboxLogDirectory(std::wstring applicationName, std::wstring partitionId, std::wstring servicePackageActivationId);

        uint64 GetNewContainerInstanceIdAtomic(std::wstring applicationName, std::wstring partitionId, std::wstring servicePackageActivationId, std::wstring codePackageName, Common::TimeSpan const timeout);

        std::wstring GetContainerRelativeLogPath(std::wstring codePackageName, uint64 containerInstanceId);

        std::wstring GetLogMountDirectory(std::wstring applicationName, std::wstring partitionId, std::wstring servicePackageActivationId);

        std::wstring GetContainerFullLogPath(std::wstring applicationName, std::wstring partitionId, std::wstring servicePackageActivationId, std::wstring codePackageName, uint64 containerInstanceId);
    }
}
