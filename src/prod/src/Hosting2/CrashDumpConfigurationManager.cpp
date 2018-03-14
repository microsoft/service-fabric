// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;

StringLiteral const TraceCrashDumpConfigurationManager("CrashDumpConfigurationManager");
const std::wstring CrashDumpConfigurationManager::CrashDumpsKeyName = L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps";
const std::wstring CrashDumpConfigurationManager::FabricCrashDumpsKeyName = L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\Fabric.exe";
const std::wstring CrashDumpConfigurationManager::ApplicationCrashDumpsFolderName = L"ApplicationCrashDumps";
const std::wstring CrashDumpConfigurationManager::DumpFolderValueName = L"DumpFolder";
const std::wstring CrashDumpConfigurationManager::DumpTypeValueName = L"DumpType";

CrashDumpConfigurationManager::CrashDumpConfigurationManager(ComponentRoot const & root)
    : RootedObject(root),
    FabricComponent(),
    crashDumpCollectionAccessDenied_(false),
    crashDumpsBaseFolder_(),
    exeToServicePackages_(),
    servicePackageToExes_(),
    nodeToServicePackages_(),
    mapLock_(),
    mapIsClosed_(false)
{
}

CrashDumpConfigurationManager::~CrashDumpConfigurationManager()
{
}

ErrorCode CrashDumpConfigurationManager::SetupServiceCrashDumpCollection(
    wstring const & nodeId,
    wstring const & servicePackageId,
    vector<wstring> const & exeNames)
{
    wstring servicePackageKey = nodeId;
    servicePackageKey.append(servicePackageId);
    {
        AcquireWriteLock writeLock(mapLock_);
        if (mapIsClosed_)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        for(auto iter = exeNames.begin(); iter != exeNames.end(); ++iter)
        {
            auto exeFindResult = exeToServicePackages_.find(*iter);
            if (exeFindResult == exeToServicePackages_.end())
            {
                auto error = EnableCrashDumpCollection(*iter);
                if (!error.IsSuccess())
                {
                    return error;
                }

                set<wstring> servicePackages;
                servicePackages.insert(servicePackageKey);
                pair<wstring, set<wstring>> exeToServicePackage;
                exeToServicePackage.first = *iter;
                exeToServicePackage.second = move(servicePackages);
                exeToServicePackages_.insert(exeToServicePackage);
            }
            else
            {
                (*exeFindResult).second.insert(servicePackageKey); 
            }

            auto servicePkgFindResult = servicePackageToExes_.find(servicePackageKey);
            if (servicePkgFindResult == servicePackageToExes_.end())
            {
                set<wstring, IsLessCaseInsensitiveComparer<wstring>> exeNamesPerServicePackage;
                exeNamesPerServicePackage.insert(*iter);
                pair<wstring, set<wstring, IsLessCaseInsensitiveComparer<wstring>>> servicePackageToExe;
                servicePackageToExe.first = servicePackageKey;
                servicePackageToExe.second = move(exeNamesPerServicePackage);
                servicePackageToExes_.insert(servicePackageToExe);
            }
            else
            {
                (*servicePkgFindResult).second.insert(*iter);
            }

            auto nodeFindResult = nodeToServicePackages_.find(nodeId);
            if (nodeFindResult == nodeToServicePackages_.end())
            {
                set<wstring> servicePackages;
                servicePackages.insert(servicePackageId);
                pair<wstring, set<wstring>> nodeToServicePackage;
                nodeToServicePackage.first = nodeId;
                nodeToServicePackage.second = move(servicePackages);
                nodeToServicePackages_.insert(nodeToServicePackage);
            }
            else
            {
                (*nodeFindResult).second.insert(servicePackageId);
            }
        }
    }

    WriteInfo(
        TraceCrashDumpConfigurationManager, 
        "Crash dump collection for service package {0} is enabled.", 
        servicePackageId);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode CrashDumpConfigurationManager::CleanupServiceCrashDumpCollectionLocked(
    wstring const & nodeId,
    wstring const & servicePackageId,
    wstring const & servicePackageKey,
    bool modifyNodeToServicePackagesMap)
{
    ErrorCode finalError(ErrorCodeValue::Success);

    if (mapIsClosed_)
    {
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    auto servicePkgFindResult = servicePackageToExes_.find(servicePackageKey);
    if (servicePkgFindResult != servicePackageToExes_.end())
    {
        set<wstring, IsLessCaseInsensitiveComparer<wstring>> const & exeNames = servicePkgFindResult->second;
        for(auto iter = exeNames.begin(); iter != exeNames.end(); ++iter)
        {
            auto exeFindResult = exeToServicePackages_.find(*iter);
            if (exeFindResult != exeToServicePackages_.end())
            {
                exeFindResult->second.erase(servicePackageKey);
                if (0 == exeFindResult->second.size())
                {
                    auto error = DisableCrashDumpCollection(*iter);
                    if (!error.IsSuccess())
                    {
                        finalError.Overwrite(error);
                        continue;
                    }
                    exeToServicePackages_.erase(*iter);
                }
            }
        }
        servicePackageToExes_.erase(servicePackageKey);
    }

    if (modifyNodeToServicePackagesMap)
    {
        auto nodeFindResult = nodeToServicePackages_.find(nodeId);
        if (nodeFindResult != nodeToServicePackages_.end())
        {
            nodeFindResult->second.erase(servicePackageId);
            if (0 == nodeFindResult->second.size())
            {
                nodeToServicePackages_.erase(nodeId);
            }
        }
    }

    if (finalError.IsSuccess())
    {
        WriteInfo(
            TraceCrashDumpConfigurationManager, 
            "Crash dump collection for service package {0} is disabled.", 
            servicePackageId);
    }
    return finalError;
}

ErrorCode CrashDumpConfigurationManager::CleanupServiceCrashDumpCollection(
    wstring const & nodeId,
    wstring const & servicePackageId)
{
    wstring servicePackageKey = nodeId;
    servicePackageKey.append(servicePackageId);

    ErrorCode error;
    {
        AcquireWriteLock writeLock(mapLock_);
        error = CleanupServiceCrashDumpCollectionLocked(nodeId, servicePackageId, servicePackageKey, true);
    }
    return error;
}

ErrorCode CrashDumpConfigurationManager::CleanupNodeCrashDumpCollectionLocked(
    wstring const & nodeId,
    bool modifyNodeToServicePackagesMap)
{
    ErrorCode error(ErrorCodeValue::Success);
    auto nodeFindResult = nodeToServicePackages_.find(nodeId);
    if (nodeFindResult != nodeToServicePackages_.end())
    {
        set<wstring> const & servicePackages = nodeFindResult->second;
        for(auto iter = servicePackages.begin(); iter != servicePackages.end(); ++iter)
        {
            wstring servicePackageKey = nodeId;
            servicePackageKey.append(*iter);
            ErrorCode currentError = CleanupServiceCrashDumpCollectionLocked(nodeId, *iter, servicePackageKey, false);
            if (!currentError.IsSuccess())
            {
                error.Overwrite(currentError);
            }
        }
        nodeFindResult->second.clear();
        if (modifyNodeToServicePackagesMap)
        {
            nodeToServicePackages_.erase(nodeId);
        }
    }
    return error;
}

ErrorCode CrashDumpConfigurationManager::CleanupNodeCrashDumpCollection(
    wstring const & nodeId)
{
    ErrorCode error;
    {
        AcquireWriteLock writeLock(mapLock_);
        error = CleanupNodeCrashDumpCollectionLocked(nodeId, true);
    }
    return error;
}

ErrorCode CrashDumpConfigurationManager::CleanupCrashDumpCollection()
{
    ErrorCode error;
    {
        AcquireWriteLock writeLock(mapLock_);
        for(auto iter = nodeToServicePackages_.begin(); iter != nodeToServicePackages_.end(); ++iter)
        {
            error = CleanupNodeCrashDumpCollectionLocked(iter->first, false);
            WriteTrace(
                error.ToLogLevel(),
                TraceCrashDumpConfigurationManager,
                "CleanupNodeCrashDumpCollection returned error {0} for {1}",
                error,
                iter->first);
        }
        nodeToServicePackages_.clear();
        mapIsClosed_ = true;
    }
    return error;
}

ErrorCode CrashDumpConfigurationManager::CreateCrashDumpsBaseFolder()
{
    // Although we only need to open the key for read at the moment,
    // we open it for write in order to check whether we have write
    // access too. This is because we will need write access later
    // and we want to determine upfront whether we have it.
#if !defined(PLATFORM_UNIX)
    RegistryKey key(FabricCrashDumpsKeyName, false);
    if (!key.IsValid)
    {
        if (ERROR_ACCESS_DENIED == key.Error)
        {
            WriteInfo(
                TraceCrashDumpConfigurationManager,
                "Crash dump collection for applications is disabled because we do not have access to the necessary registry keys.");
            crashDumpCollectionAccessDenied_ = true;
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            WriteError(
                TraceCrashDumpConfigurationManager,
                "Error {0} while opening registry key {1}.",
                key.Error,
                FabricCrashDumpsKeyName);
            return ErrorCode::FromWin32Error(key.Error);
        }
    }
    wstring fabricCrashDumpsPath;
    if (!key.GetValue(DumpFolderValueName, fabricCrashDumpsPath, true))
    {
        WriteError(
            TraceCrashDumpConfigurationManager, 
            "Unable to read value {0} in registry key {1} because of error {2}",
            DumpFolderValueName,
            FabricCrashDumpsKeyName,
            key.Error);
        return ErrorCode::FromWin32Error(key.Error);
    }

    wstring logFolder = Path::GetDirectoryName(fabricCrashDumpsPath);
    crashDumpsBaseFolder_ = Path::Combine(logFolder, ApplicationCrashDumpsFolderName);
#else
    crashDumpsBaseFolder_ = L".";
#endif

    if (!Directory::Exists(crashDumpsBaseFolder_))
    {
        ErrorCode error = Directory::Create2(crashDumpsBaseFolder_);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceCrashDumpConfigurationManager, 
                "Unable to create directory '{0}'. Error: {1}.", 
                crashDumpsBaseFolder_, 
                error);
            return error;
        }
    }

    WriteInfo(
        TraceCrashDumpConfigurationManager, 
        "Created directory '{0}' for application crash dumps.", 
        crashDumpsBaseFolder_);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode CrashDumpConfigurationManager::EnableCrashDumpCollection(
    wstring const & exeName)
{
    wstring crashDumpFolder = Path::Combine(crashDumpsBaseFolder_, exeName);
    if (!Directory::Exists(crashDumpFolder))
    {
        ErrorCode error = Directory::Create2(crashDumpFolder);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceCrashDumpConfigurationManager, 
                "Unable to create directory '{0}'. Error: {1}.", 
                crashDumpFolder, 
                error);
            return error;
        }
    }

    wstring keyName = Path::Combine(CrashDumpsKeyName, exeName);
    RegistryKey key(keyName, false);
    if (!key.IsValid)
    {
        WriteError(
            TraceCrashDumpConfigurationManager, 
            "Unable to create registry key {0} because of error {1}",
            keyName,
            key.Error);
        return ErrorCode::FromWin32Error(key.Error);
    }
    ErrorCode error(ErrorCodeValue::Success);
    if (!key.SetValue(DumpFolderValueName, crashDumpFolder))
    {
        WriteError(
            TraceCrashDumpConfigurationManager, 
            "Unable to write value {0} in registry key {1} because of error {2}",
            DumpFolderValueName,
            keyName,
            key.Error);
        error = ErrorCode::FromWin32Error(key.Error);
    }
    if (error.IsSuccess())
    {
        if (!key.SetValue(DumpTypeValueName, DumpTypeFullDump))
        {
            WriteError(
                TraceCrashDumpConfigurationManager, 
                "Unable to write value {0} in registry key {1} because of error {2}",
                DumpTypeValueName,
                keyName,
                key.Error);
            error = ErrorCode::FromWin32Error(key.Error);
        }
    }

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceCrashDumpConfigurationManager, 
            "Crash dumps for {0} will be created in directory '{1}'.", 
            exeName,
            crashDumpFolder);
    }
    else
    {
        DisableCrashDumpCollection(exeName);
    }
    return error;
}

ErrorCode CrashDumpConfigurationManager::DisableCrashDumpCollection(
    wstring const & exeName)
{
    wstring keyName = Path::Combine(CrashDumpsKeyName, exeName);
    RegistryKey key(keyName, false);
    if (!key.IsValid)
    {
        WriteError(
            TraceCrashDumpConfigurationManager, 
            "Unable to open registry key {0} because of error {1}",
            keyName,
            key.Error);
        return ErrorCode::FromWin32Error(key.Error);
    }
    if (!key.DeleteKey())
    {
        WriteError(
            TraceCrashDumpConfigurationManager, 
            "Unable to delete registry key {0} because of error {1}",
            keyName,
            key.Error);
        return ErrorCode::FromWin32Error(key.Error);
    }
    WriteInfo(
        TraceCrashDumpConfigurationManager,
        "Registry key {0} deleted.",
        keyName);
    return ErrorCode::FromWin32Error(ErrorCodeValue::Success);
}

void CrashDumpConfigurationManager::ProcessConfigureCrashDumpRequest(
    __in Transport::Message & message, 
    __in Transport::IpcReceiverContextUPtr & context)
{
    ErrorCode error;
    ConfigureCrashDumpRequest request;
    if (!message.GetBody<ConfigureCrashDumpRequest>(request))
    {
        error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceCrashDumpConfigurationManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping, error: {1}",
            message,
            error);
    }
    else
    {
        if (crashDumpCollectionAccessDenied_)
        {
            WriteInfo(
                TraceCrashDumpConfigurationManager,
                "Crash dump configuration request could not be processed because we do not have access to the necessary registry keys. Node: {0}, Service package:{1}.",
                request.NodeId,
                request.ServicePackageId);
            error = ErrorCodeValue::Success;
        } 
        else 
        {
#if !defined(PLATFORM_UNIX)
            if (request.Enable)
            {
                error = SetupServiceCrashDumpCollection(request.NodeId, request.ServicePackageId, request.ExeNames);
            }
            else
            {
                if (!request.ServicePackageId.empty())
                {
                    error = CleanupServiceCrashDumpCollection(request.NodeId, request.ServicePackageId);
                }
                else
                {
                    error = CleanupNodeCrashDumpCollection(request.NodeId);
                }
            }
#endif
        }
    }
    SendConfigureCrashDumpReply(error, context);
}

void CrashDumpConfigurationManager::SendConfigureCrashDumpReply(
    ErrorCode const & error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    auto replyBody = make_unique<ConfigureCrashDumpReply>(error);
    Transport::MessageUPtr reply = make_unique<Transport::Message>(*replyBody);
    WriteNoise(
        TraceCrashDumpConfigurationManager,
        Root.TraceId,
        "Sending ConfigureCrashDumpReply: ReplyBody={0}",
        *reply);
    context->Reply(move(reply));
}

ErrorCode CrashDumpConfigurationManager::OnOpen()
{
    ErrorCode error = CreateCrashDumpsBaseFolder();
    if (crashDumpCollectionAccessDenied_)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
    return ErrorCode(error);
}

ErrorCode CrashDumpConfigurationManager::OnClose()
{
    if (crashDumpCollectionAccessDenied_)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
    return CleanupCrashDumpCollection();
}

void CrashDumpConfigurationManager::OnAbort()
{
    if (crashDumpCollectionAccessDenied_)
    {
        return;
    }
    CleanupCrashDumpCollection();
}

bool CrashDumpConfigurationManager::IsCrashDumpCollectionEnabled(
        wstring const & exeName)
{
    wstring keyName = Path::Combine(CrashDumpsKeyName, exeName);
    RegistryKey key(keyName, true, true);
    if (!key.IsValid)
    {
        if ((key.Error != ERROR_PATH_NOT_FOUND) && (key.Error != ERROR_FILE_NOT_FOUND))
        {
            WriteWarning(
                TraceCrashDumpConfigurationManager, 
                "Unable to open registry key {0} because of error {1}. We will assume that crash dump collection is not enabled.",
                keyName,
                key.Error);
        }
        return false;
    }
    wstring exeCrashDumpPath;
    if (!key.GetValue(DumpFolderValueName, exeCrashDumpPath))
    {
        if ((key.Error != ERROR_PATH_NOT_FOUND) && (key.Error != ERROR_FILE_NOT_FOUND))
        {
            WriteWarning(
                TraceCrashDumpConfigurationManager, 
                "Unable to read value {0} in registry key {1} because of error {2}. We will assume that crash dump collection is not enabled.",
                DumpFolderValueName,
                keyName,
                key.Error);
        }
        return false;
    }
    DWORD dumpType;
    if (!key.GetValue(DumpTypeValueName, dumpType))
    {
        if ((key.Error != ERROR_PATH_NOT_FOUND) && (key.Error != ERROR_FILE_NOT_FOUND))
        {
            WriteWarning(
                TraceCrashDumpConfigurationManager, 
                "Unable to read value {0} in registry key {1} because of error {2}. We will assume that crash dump collection is not enabled.",
                DumpTypeValueName,
                keyName,
                key.Error);
        }
        return false;
    }
    return true;
}
