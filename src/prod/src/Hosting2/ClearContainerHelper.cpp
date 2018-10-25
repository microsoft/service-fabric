// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <algorithm>
#include <fstream>

using namespace Hosting2;
using namespace Common;

StringLiteral const TraceType("ContainerActivator");

// Constants
std::wstring const ClearContainerHelper::ClearContainerLogHelperConstants::FabricApplicationNamePrefix(L"fabric:/");
std::wstring const ClearContainerHelper::ClearContainerLogHelperConstants::ApplicationNameSuffix(L"_clearcontainer");
std::wstring const ClearContainerHelper::ClearContainerLogHelperConstants::CodePackageInstanceIdFileName(L"codepackageinstanceid");
std::wstring const ClearContainerHelper::ClearContainerLogHelperConstants::ContainerLogFileName(L"application.log");
std::wstring const ClearContainerHelper::ClearContainerLogHelperConstants::CodePackageDirectoryName(L"codepackges");
char const ClearContainerHelper::ClearContainerLogHelperConstants::PathSeparator = '/';
char const ClearContainerHelper::ClearContainerLogHelperConstants::PathSeparatorReplacer = '_';

// Configuration to get dnsservice scenario working for clear containers.
// 1. Setting the nameserver value inside the container to host IP address (where DNS service is running) other than    
// on CNI bridge network was resulting into DNS responses being rejected.
// They were getting rejected due to address mismatch resulting from translation.
// The subnet being passed as part of CNI plugin is hardcoded to 10.88.0.0/16. Hence Host IP address on 
// the CNI bridge network will be always "10.88.0.1". 
// 2. ndots field value is hardcoded 1 to avoid any issue arising from clear container runtime default behavior.
std::string const ClearContainerHelper::ClearContainerLogHelperConstants::DnsServiceIP4Address("10.88.0.1");
std::string const ClearContainerHelper::ClearContainerLogHelperConstants::NDotsConfig("ndots:1");

#pragma region UsingContainerDescription
std::wstring ClearContainerHelper::GetSimpleApplicationName(ContainerDescription const & containerDescription)
{
    // this is the service fabric application name without the fabric:/ prefix
    return ClearContainerHelper::GetSimpleApplicationName(containerDescription.ApplicationName);
}

std::wstring ClearContainerHelper::GetDigestedApplicationName(Hosting2::ContainerDescription const & containerDescription)
{
    // Digested application name is the application name without the fabric:/ prefix and
    // an additional suffix of _clearcontainer for clear containers
    return ClearContainerHelper::GetDigestedApplicationName(containerDescription.ApplicationName);
}

std::wstring ClearContainerHelper::GetSandboxLogDirectory(Hosting2::ContainerDescription const & containerDescription)
{
    // Sandbox log directory. Containers of a sandbox has container log paths relative to this directory.
    // Format:<root directory set by clustermanifest>/<ApplicationName>/<PartitionId>/<ServicePackageActivationId>/
    return ClearContainerHelper::GetSandboxLogDirectory(
                containerDescription.ApplicationName,
                containerDescription.PartitionId,
                containerDescription.ServicePackageActivationId);
}

uint64 ClearContainerHelper::GetNewContainerInstanceIdAtomic(
    Hosting2::ContainerDescription const & containerDescription,
    Common::TimeSpan const timeout)
{
    return ClearContainerHelper::GetNewContainerInstanceIdAtomic(
                containerDescription.ApplicationName,
                containerDescription.PartitionId,
                containerDescription.ServicePackageActivationId,
                containerDescription.CodePackageName,
                timeout);
}

std::wstring ClearContainerHelper::GetContainerRelativeLogPath(
    Hosting2::ContainerDescription const & containerDescription,
    uint64 containerInstanceId)
{
    // Log path for container is relative to sandbox log directory.
    // Relative log path format: <codePackageName>/application.log
    // Full path to logs is sandbox.logdirectory + container.logPath
    return ClearContainerHelper::GetContainerRelativeLogPath(containerDescription.CodePackageName, containerInstanceId);
}

std::wstring ClearContainerHelper::GetLogMountDirectory(Hosting2::ContainerDescription const & containerDescription)
{
    return ClearContainerHelper::GetLogMountDirectory(
                containerDescription.ApplicationName,
                containerDescription.PartitionId,
                containerDescription.ServicePackageActivationId);
}

std::wstring ClearContainerHelper::GetContainerFullLogPath(
    Hosting2::ContainerDescription const & containerDescription,
    uint64 instanceId)
{
    return ClearContainerHelper::GetContainerFullLogPath(
                containerDescription.ApplicationName,
                containerDescription.PartitionId,
                containerDescription.ServicePackageActivationId,
                containerDescription.CodePackageName,
                instanceId);
}
#pragma endregion

#pragma region UsingRaw
std::wstring ClearContainerHelper::GetSimpleApplicationName(std::wstring applicationName)
{
    // Simple Application Name is the application name without the fabric:/ prefix

    std::wstring applicationNamePart = applicationName;

    // strip fabric:/ if it exists in name
    if (ClearContainerHelper::ClearContainerLogHelperConstants::FabricApplicationNamePrefix.length() <= applicationNamePart.length() &&
        std::equal(ClearContainerHelper::ClearContainerLogHelperConstants::FabricApplicationNamePrefix.begin(), ClearContainerHelper::ClearContainerLogHelperConstants::FabricApplicationNamePrefix.end(), applicationNamePart.begin()))
    {
        // ApplicationName contains fabric:/ at the begining which would end up creating a directory in the path
        applicationNamePart = applicationNamePart.substr(ClearContainerHelper::ClearContainerLogHelperConstants::FabricApplicationNamePrefix.length());
    }

    // replace / with _ to prevent additional directories in log path
    std::replace(applicationNamePart.begin(), applicationNamePart.end(), ClearContainerHelper::ClearContainerLogHelperConstants::PathSeparator, ClearContainerHelper::ClearContainerLogHelperConstants::PathSeparatorReplacer);

    return applicationNamePart;
}

std::wstring ClearContainerHelper::GetDigestedApplicationName(std::wstring applicationName)
{
    // Digested application name is the application name without the fabric:/ prefix and
    // a suffix of _clearcontainer for clear containers

    auto simpleApplicationName = ClearContainerHelper::GetSimpleApplicationName(applicationName);

    return simpleApplicationName + ClearContainerHelper::ClearContainerLogHelperConstants::ApplicationNameSuffix;
}

std::wstring ClearContainerHelper::GetSandboxLogDirectory(
    std::wstring applicationName,
    std::wstring partitionId,
    std::wstring servicePackageActivationId)
{
    // Sandbox log directory. Containers of a sandbox has container log paths relative to this directory.
    // Format:<root directory set by clustermanifest>/<ApplicationName>/<PartitionId>/<ServicePackageActivationId>/
    std::wstring logDirectory = L"/mnt/logs"; // TODO read this value from the cluster manifest
    auto digestedApplicationName = ClearContainerHelper::GetDigestedApplicationName(applicationName);

    Path::CombineInPlace(logDirectory, digestedApplicationName);
    Path::CombineInPlace(logDirectory, partitionId);
    Path::CombineInPlace(logDirectory, servicePackageActivationId);

    return logDirectory;
}

uint64 ClearContainerHelper::GetNewContainerInstanceIdAtomic(
    std::wstring applicationName,
    std::wstring partitionId,
    std::wstring servicePackageActivationId,
    std::wstring codePackageName,
    Common::TimeSpan const timeout)
{
    // Atomically get a new instance id for the container. This id is needed to get the relative log path and full

    auto logMountDirectory = ClearContainerHelper::GetLogMountDirectory(applicationName, partitionId, servicePackageActivationId);
    auto containerRootLogDirectory = Path::Combine(logMountDirectory, codePackageName);
    uint64 containerInstanceId = 0;

    File containerInstanceIdFile;
    auto containerInstanceIdFilePath = Path::Combine(containerRootLogDirectory, ClearContainerHelper::ClearContainerLogHelperConstants::CodePackageInstanceIdFileName);

    ErrorCode error;

    if (!File::Exists(containerInstanceIdFilePath))
    {
        error = Directory::Create2(Path::GetDirectoryName(containerInstanceIdFilePath));

        if (error.IsSuccess())
        {
            error = File::Touch(containerInstanceIdFilePath);
        }

        if (!error.IsSuccess())
        {
            // TODO
        }
    }
    else
    {
        // read current container instance id
        ifstream containerIdFile(StringUtility::Utf16ToUtf8(containerInstanceIdFilePath));
        if (containerIdFile.is_open())
        {
            string line;
            containerIdFile >> line;
            containerInstanceId = std::strtoull(line.c_str(), NULL, 10);

            containerIdFile.close();
        }

        // get new container instance id
        // it is possible there already is a directory for the $(current instance id + 1), in which case it would have been
        // a bug in this module or externally created. For now we'll assume no such things are happening
        containerInstanceId += 1;
    }

    // save new instance id
    ofstream containerIdFile(StringUtility::Utf16ToUtf8(containerInstanceIdFilePath));
    if (containerIdFile.is_open())
    {
        containerIdFile << containerInstanceId << "\n";

        containerIdFile.close();
    }

    return containerInstanceId;
}

std::wstring ClearContainerHelper::GetContainerRelativeLogPath(std::wstring codePackageName, uint64 containerInstanceId)
{
    // Log path for container is relative to sandbox log directory.
    // Relative log path format: codepackages/<codePackageName>/<instance id>/application.log
    // Full path to logs is sandbox.logdirectory + container.logPath
    return Path::Combine(ClearContainerHelper::ClearContainerLogHelperConstants::CodePackageDirectoryName,
                Path::Combine(Path::Combine(codePackageName, std::to_wstring(containerInstanceId)),
                              ClearContainerHelper::ClearContainerLogHelperConstants::ContainerLogFileName));
}

std::wstring ClearContainerHelper::GetLogMountDirectory(
    std::wstring applicationName,
    std::wstring partitionId,
    std::wstring servicePackageActivationId)
{
    return Path::Combine(
                ClearContainerHelper::GetSandboxLogDirectory(applicationName, partitionId, servicePackageActivationId),
                ClearContainerHelper::ClearContainerLogHelperConstants::CodePackageDirectoryName);
}


std::wstring ClearContainerHelper::GetContainerFullLogPath(
    std::wstring applicationName,
    std::wstring partitionId,
    std::wstring servicePackageActivationId,
    std::wstring codePackageName,
    uint64 containerInstanceId)
{
    return Path::Combine(
            ClearContainerHelper::GetSandboxLogDirectory(
                applicationName,
                partitionId,
                servicePackageActivationId),
            ClearContainerHelper::GetContainerRelativeLogPath(codePackageName, containerInstanceId));
}
#pragma endregion
