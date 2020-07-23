// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

StringLiteral TraceType_SMBShareUtility = "SMBShareUtility";

GlobalWString SMBShareUtility::WindowsFabricSMBShareRemark = make_global<wstring>(L"WindowsFabric share");

GlobalWString SMBShareUtility::LanmanServerParametersRegistryPath = make_global<wstring>(L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters");
GlobalWString SMBShareUtility::NullSessionSharesRegistryValue = make_global<wstring>(L"NullSessionShares");

GlobalWString SMBShareUtility::LsaRegistryPath = make_global<wstring>(L"SYSTEM\\CurrentControlSet\\Control\\Lsa");
GlobalWString SMBShareUtility::EveryoneIncludesAnonymousRegistryValue = make_global<wstring>(L"EveryoneIncludesAnonymous");

ErrorCode SMBShareUtility::CreateShare(
    std::wstring const & localPath, 
    std::wstring const & shareName, 
    SecurityDescriptorSPtr const & securityDescriptor)    
{    
    wstring qualifiedLocalPath;
    auto error = Path::QualifyPath(localPath, qualifiedLocalPath);
    if(!error.IsSuccess())
    {
        return error;
    }

    // check for existing share
    PSHARE_INFO_502 shareInfoPtr;
    NET_API_STATUS result = NetShareGetInfo(NULL /*local machine*/, (LPWSTR) shareName.c_str(), 502, (LPBYTE *) &shareInfoPtr);
    if(result == NERR_Success)
    {
        if(shareInfoPtr->shi502_remark == *WindowsFabricSMBShareRemark && 
            shareInfoPtr->shi502_type == STYPE_DISKTREE)
        {
            if(securityDescriptor->IsEquals(shareInfoPtr->shi502_security_descriptor))
            {
                return ErrorCodeValue::Success;
            }

            auto innerError = DeleteShare(shareName);
            if(!innerError.IsSuccess()) { return innerError; }            
        }
        else
        {
            return ErrorCodeValue::OperationFailed;
        }
    }
    else if(result != NERR_NetNameNotFound)
    {
        return ErrorCode::FromWin32Error(result);
    }

    SHARE_INFO_502 shareInfo;
    shareInfo.shi502_netname = (LPWSTR) shareName.c_str();
    shareInfo.shi502_path = (LPWSTR) qualifiedLocalPath.c_str();
    shareInfo.shi502_remark = (LPWSTR) WindowsFabricSMBShareRemark->c_str();
    shareInfo.shi502_type = STYPE_DISKTREE;    
    shareInfo.shi502_max_uses = (DWORD)-1 /*unlimited connections allowed*/;
    shareInfo.shi502_security_descriptor = securityDescriptor->PSecurityDescriptor;
    shareInfo.shi502_passwd = L"";

    DWORD paramErr;
    result = NetShareAdd(NULL /*local machine*/, 502, (LPBYTE) &shareInfo, &paramErr);

    return ErrorCode::FromWin32Error(result);
}

ErrorCode SMBShareUtility::DeleteShare(std::wstring const & shareName)
{
    NET_API_STATUS result = NetShareDel(NULL /*local machine*/, (LPWSTR) shareName.c_str(), 0);
    return ErrorCode::FromWin32Error(result); 
}

set<wstring> SMBShareUtility::DeleteOrphanShares()
{
    set<wstring> deletedShares;
    PSHARE_INFO_2 bufferPtr, currentPtr;
    DWORD entriesCount, totalCount, resume;
    NET_API_STATUS result;
    do
    {
        result = NetShareEnum(NULL, 2, (LPBYTE *) &bufferPtr, MAX_PREFERRED_LENGTH, &entriesCount, &totalCount, &resume);
        if(result == ERROR_SUCCESS || result == ERROR_MORE_DATA)
        {
            currentPtr = bufferPtr;
            for(DWORD i=0; i<entriesCount; i++)
            {
                if(StringUtility::AreEqualCaseInsensitive(currentPtr->shi2_remark, *WindowsFabricSMBShareRemark) &&
                    !Directory::Exists(currentPtr->shi2_path))
                {
                    auto error = DeleteShare(currentPtr->shi2_netname);                        
                    if(error.IsSuccess())
                    {
                        deletedShares.insert(currentPtr->shi2_netname);
                    }
                }
                currentPtr++;
            }
            NetApiBufferFree(bufferPtr);
        }
    }while(result == ERROR_MORE_DATA);

    return deletedShares;
}

ErrorCode SMBShareUtility::EnableAnonymousAccess(wstring const & localPath, wstring const & shareName, DWORD accessMask, SidSPtr const & anonymousSid, TimeSpan const & timeout)
{
	TimeoutHelper timeoutHelper(timeout);

    // Enable Anonymous access on NTFS folder
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    principalPermissions.push_back(make_pair(anonymousSid, accessMask));
    auto error = SecurityUtility::OverwriteFolderACL(
        localPath,
        principalPermissions,
        false,
        true,
		timeoutHelper.GetRemainingTime());
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType_SMBShareUtility,
        "ACL folder '{0}' for anonymous access. Error:{1}",
        localPath, error);
    if(!error.IsSuccess())
    {
        return error;
    }

    // Add shareName to Registry
    RegistryKey lanmanRegistryKey(SMBShareUtility::LanmanServerParametersRegistryPath, false);
    if(!lanmanRegistryKey.IsValid)
    {
        Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: Failed to open registry key to LanmanServerParametersRegistryPath");
        return ErrorCode::FromWin32Error(lanmanRegistryKey.Error);
    }

    RegistryKey lsaRegistryKey(SMBShareUtility::LsaRegistryPath, false);
    if(!lsaRegistryKey.IsValid)
    {
        Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: Failed to open registry key to LsaRegistryPath");
        return ErrorCode::FromWin32Error(lsaRegistryKey.Error);
    }

    {
        // synchronize access for scale-min scenario using named mutex
        auto mutexName = L"FileStoreServiceSetupMutex";
        auto mutexHandle = MutexHandle::CreateUPtr(mutexName);
		auto err = mutexHandle->WaitOne(timeoutHelper.GetRemainingTime());
        if (!err.IsSuccess())
        {
            Trace.WriteWarning(TraceType_SMBShareUtility, "wait on mutex {0} failed: {1}", mutexName, err);
            return ErrorCodeValue::OperationFailed;
        }

        vector<wstring> existingShareNames;
        lanmanRegistryKey.GetValue(SMBShareUtility::NullSessionSharesRegistryValue, existingShareNames);
        if(!lanmanRegistryKey.IsValid && lanmanRegistryKey.Error != ERROR_FILE_NOT_FOUND)
        {
            Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: Failed to get registry value of NullSessionSharesRegistryValue");
            return ErrorCode::FromWin32Error(lanmanRegistryKey.Error);
        }

        for(auto const & existingShareName : existingShareNames)
        {
            if(StringUtility::AreEqualCaseInsensitive(shareName, existingShareName))
            {
                // shareName already in registry                
                return ErrorCodeValue::Success;
            }
        }

        existingShareNames.push_back(shareName);

        if(!lanmanRegistryKey.SetValue(SMBShareUtility::NullSessionSharesRegistryValue, existingShareNames))
        {
            Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: Failed to set registry value in NullSessionSharesRegistryValue");
            return ErrorCode::FromWin32Error(lanmanRegistryKey.Error);
        }

        if(!lsaRegistryKey.SetValue(SMBShareUtility::EveryoneIncludesAnonymousRegistryValue, 1))
        {
            Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: Failed to set registry value in EveryoneIncludesAnonymousRegistryValue");
            return ErrorCode::FromWin32Error(lsaRegistryKey.Error);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode SMBShareUtility::DisableAnonymousAccess(wstring const & localPath, wstring const & shareName, TimeSpan const & timeout)
{
	TimeoutHelper timeoutHelper(timeout);

    SidSPtr anonymousSid;
    auto error = BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE::WinAnonymousSid, anonymousSid);
    if(!error.IsSuccess())
    {
        Trace.WriteWarning(TraceType_SMBShareUtility, "DisableAnonymousAccess(): Failed to create sid object for WELL_KNOWN_SID_TYPE::WinAnonymousSid. Error:{0}", error);
        return error;
    }

    // Enable Anonymous access on NTFS folder
    error = SecurityUtility::RemoveFolderAcl(
        anonymousSid,
        localPath,
        false,
		timeoutHelper.GetRemainingTime());
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType_SMBShareUtility,
        "ACL folder '{0}' for anonymous access. Error:{1}",
        localPath, error);

    RegistryKey registryKey(SMBShareUtility::LanmanServerParametersRegistryPath, false);
    if(!registryKey.IsValid)
    {
        return ErrorCode::FromWin32Error(registryKey.Error);
    }

    {
        // synchronize access for scale-min scenario using named mutex
        auto mutexName = L"FileStoreServiceSetupMutex";
        auto mutexHandle = MutexHandle::CreateUPtr(mutexName);
        auto err = mutexHandle->WaitOne(timeoutHelper.GetRemainingTime());
        if (!err.IsSuccess())
        {
            Trace.WriteWarning(TraceType_SMBShareUtility, "SetupShare: wait on mutex {0} failed: {1}", mutexName, err); 
            return ErrorCodeValue::OperationFailed;
        }

        vector<wstring> existingShareNames;
        if(!registryKey.GetValue(SMBShareUtility::NullSessionSharesRegistryValue, existingShareNames) && registryKey.Error != ERROR_FILE_NOT_FOUND)
        {
            return ErrorCode::FromWin32Error(registryKey.Error);
        }

        bool matchFound = false;
        vector<wstring> newShareNames;
        for(auto const & existingShareName : existingShareNames)
        {
            if(existingShareName == shareName)
            {
                matchFound = true;
            }
            else
            {
                newShareNames.push_back(existingShareName);
            }
        }

        if(matchFound)
        {
            if(!registryKey.SetValue(SMBShareUtility::NullSessionSharesRegistryValue, newShareNames))
            {
                return ErrorCode::FromWin32Error(registryKey.Error);
            }
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode SMBShareUtility::EnsureServerServiceIsRunning()
{
    ErrorCode errorCode(ErrorCodeValue::Success);
    bool serviceRunning = false;

    auto scmHandle = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE serviceHandle = NULL;
    if(scmHandle != NULL)
    {
        wstring serviceName = L"LanmanServer";
        serviceHandle = ::OpenService(scmHandle, serviceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);
        if(serviceHandle != NULL)
        {
            SERVICE_STATUS status;
            if(::QueryServiceStatus(serviceHandle, &status))
            {
                if(status.dwCurrentState == SERVICE_RUNNING)
                {
                    serviceRunning = true;
                }
                else
                {
                    Trace.WriteInfo(
                        TraceType_SMBShareUtility,
                        "Service {0} is in {1} state, starting now",
                        serviceName,
                        status.dwCurrentState);

                    if(!::StartService(serviceHandle, 0, NULL))
                    {
                        errorCode = ErrorCode::FromWin32Error();
                        Trace.WriteWarning(
                            TraceType_SMBShareUtility,
                            "Failed to start service {0}, error {1}",
                            serviceName,
                            errorCode);
                    }
                    else
                    {
                        // Check if the service reached Running state within 5 seconds
                        int retryCount = 0;
                        do
                        {
                            serviceRunning = ::QueryServiceStatus(serviceHandle, &status) && status.dwCurrentState == SERVICE_RUNNING;
                            if(!serviceRunning) { ::Sleep(100); }
                        } while(!serviceRunning && ++retryCount < 50);
                    }
                }
            }
            else
            {
                errorCode = ErrorCode::FromWin32Error();
                Trace.WriteWarning(
                    TraceType_SMBShareUtility,
                    "Failed to query service state, error {0}",
                    errorCode);
            }
        }
        else
        {
            errorCode = ErrorCode::FromWin32Error();
            Trace.WriteWarning(
                TraceType_SMBShareUtility,
                "Failed to open service error {0}",
                errorCode);
        }
    }
    else
    {
        errorCode = ErrorCode::FromWin32Error();
        Trace.WriteWarning(
            TraceType_SMBShareUtility,
            "Failed to open servicecontrolmanager, error {0}",
            errorCode);
    }

    if(serviceHandle != NULL && !CloseServiceHandle(serviceHandle))
    {
        auto error = ErrorCode::FromWin32Error();
        Trace.WriteWarning(
            TraceType_SMBShareUtility,
            "Failed to close service handle,  error {0}",
            error);
    }

    if(scmHandle != NULL && !CloseServiceHandle(scmHandle))
    {
        auto error = ErrorCode::FromWin32Error();
        Trace.WriteWarning(
            TraceType_SMBShareUtility,
            "Failed to close SCM handle,  error {0}",
            error);
    }

    if(errorCode.IsSuccess() && !serviceRunning)
    {
        Trace.WriteWarning(
            TraceType_SMBShareUtility,
            "The service has been started, but did not transition to 'Running' state.");

        errorCode.Overwrite(ErrorCodeValue::InvalidState);
    }

    return errorCode;
}



