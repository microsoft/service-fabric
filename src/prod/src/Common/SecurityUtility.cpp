// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <libgen.h>
#endif

using namespace std;
using namespace Common;

StringLiteral TraceType_SecurityUtility = "SecurityUtility";

wstring const SecurityUtility::DaclLockFileName(L"daclupdate.lock");
wstring const SecurityUtility::DaclStampFileName(L"daclupdate.aclstamp");

#if defined(PLATFORM_UNIX)

int SecurityUtility::GetPwByUid(uid_t uid, passwd & pwd)
{
    auto pwBufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pwBufSize == -1) pwBufSize = 16*1024;

    auto pwBuf = (char*)malloc(pwBufSize);
    if (!pwBuf) return ENOMEM;
    KFinally([=] { free(pwBuf); });

    passwd* match = nullptr;
    return getpwuid_r(uid, &pwd, pwBuf, pwBufSize, &match); 
}

int SecurityUtility::GetGrByGid(gid_t gid, group & grp)
{
    auto grBufSize = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (grBufSize == -1) grBufSize = 16*1024;

    auto grBuf = (char*)malloc(grBufSize);
    if (!grBuf) return ENOMEM;
    KFinally([=] { free(grBuf); });

    group* match = nullptr;
    return getgrgid_r(gid, &grp, grBuf, grBufSize, &match);
}

ErrorCode SecurityUtility::EnsurePrivilege(std::wstring const & privilege)
{
    return ErrorCode::Success();
}
ErrorCode SecurityUtility::UpdateFileAcl(
    vector<SidSPtr> const & sids,
    wstring const & fileName, 
    DWORD const accessMask,
    TimeSpan const timeout)
{
    return ErrorCode::Success();
}
ErrorCode SecurityUtility::UpdateFolderAcl(
    Common::SidSPtr const & sid,
    std::wstring const & folderName,
    DWORD const accessMask,
    Common::TimeSpan const timeout)
{
    return ErrorCode::Success();
}
ErrorCode SecurityUtility::UpdateFolderAcl(
    vector<SidSPtr> const & sids,
    wstring const & folderName,
    DWORD const accessMask,
    TimeSpan const timeout,
    bool removeInvalidSids)
{
    string cmd, dirname;
    StringUtility::Utf16ToUtf8(folderName, dirname);
    std::replace(dirname.begin(), dirname.end(), '\\', '/');

#if defined(LINUX_ACL_ENABLED)
    cmd = string("setfacl -R ");
    for(auto iter = sids.begin(); iter != sids.end(); ++iter)
    {
        string access;
        if (accessMask & GENERIC_READ)
            access += "rx";
        if (accessMask & GENERIC_WRITE)
            access += "w";
        if (accessMask & GENERIC_ALL)
            access = "rwx";

        string modifier(" -m \"");
        PLSID psid = (PLSID)(*iter)->PSid;
        if(psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_RID)
        {
            ULONG id = psid->SubAuthority[1];
            modifier += "u:" + std::to_string(id) + ":" + access;
            modifier += ",d:u:" + std::to_string(id) + ":" + access;
        }
        else if(psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID)
        {
            ULONG id = psid->SubAuthority[1];
            modifier += "g:" + std::to_string(id) + ":" + access;
            modifier += ",d:g:" + std::to_string(id) + ":" + access;
        }
        modifier += "\"";
        cmd += modifier + " ";
    }
    cmd += " " + dirname + " > /dev/null 2>&1";
    system(cmd.c_str());
#else
    for(auto iter = sids.begin(); iter != sids.end(); ++iter)
    {
        string access;
        if (accessMask & GENERIC_READ)
            access += "rx";
        if (accessMask & GENERIC_WRITE)
            access += "w";
        if (accessMask & GENERIC_ALL)
            access = "rwx";
        PLSID psid = (PLSID) (*iter)->PSid;
        ULONG id = psid->SubAuthority[1];
        bool isGroup = (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID);
        TraceInfo(TraceTaskCodes::Common, TraceType_SecurityUtility,
                     "UpdateFolderAcl: {0} {1}: {2}, Folder {3}",
                     isGroup? "GID" : "UID",
                     id,
                     access,
                     folderName);

        // skip root
        if ((0 == id) && !isGroup) {
            continue;
        }
        // try to figure out user-owner/group-owner
        if (accessMask & GENERIC_ALL) {
            PLSID psid = (PLSID) (*iter)->PSid;
            ULONG id = psid->SubAuthority[1];
            cmd = string("chown -R ");
            if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID) {
                cmd += ":";
            }
            cmd += std::to_string(id) + " " + dirname + " > /dev/null 2>&1";
            system(cmd.c_str());
            cmd = string("chmod -R g+rwx ") + dirname + " > /dev/null 2>&1";
            system(cmd.c_str());
        }
    }
#endif
    return ErrorCode::Success();
}
ErrorCode SecurityUtility::UpdateProcessAcl(
    ProcessHandle const & processHandle,
    Common::SidSPtr const & sid,
    ACCESS_MASK accessMask)
{
    return ErrorCode::Success();
}
ErrorCode SecurityUtility::IsCurrentUserAdmin(bool & isAdminUser)
{
    isAdminUser = geteuid() == 0;
    return ErrorCode::Success();
}

ErrorCode SecurityUtility::OverwriteFolderACL(
    wstring const & folderName,
    vector<pair<SidSPtr, DWORD>> const & principalPermissions,
    bool disableInheritence,
    bool ignoreInheritenceFlag,
    TimeSpan const & timeout)
{
    ErrorCode error = ErrorCode::Success();

    if (!Directory::Exists(folderName))
    {
        TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "UpdateFolderAcl: Folder {0} does not exist.",
                folderName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    bool removeall = true;
    for (auto iter = principalPermissions.begin(); iter != principalPermissions.end(); ++iter)
    {
        vector<SidSPtr> sids = {(*iter).first};
        error = UpdateFolderAcl(sids, folderName,(*iter).second, timeout, removeall);
        if (!error.IsSuccess())
        {
            break;
        }
        removeall = false;
    }
    return error;
}

ErrorCode SecurityUtility::OverwriteFileAcl(
    vector<SidSPtr> const & sids,
    wstring const & fileName,
    DWORD const accessMask,
    TimeSpan const timeout)
{
    auto error = ErrorCode::Success();
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    for (auto const & sid : sids)
    {
        principalPermissions.push_back(make_pair(sid, accessMask));
    }

    auto removeall = true;
    for (auto const & principalPermission: principalPermissions)
    {
        auto sids = {(principalPermission).first};
        error = UpdateFolderAcl(sids, fileName,(principalPermission).second, timeout, removeall);
        if (!error.IsSuccess())
        {
            break;
        }
        removeall = false;
    }

    return error;
}

ErrorCode SecurityUtility::InstallCoreFxCertificate(
    wstring const & userName,
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType)
{
    CertContexts certContexts;
    ErrorCode error = CryptoUtility::GetCertificate(X509StoreLocation::LocalMachine, x509StoreName, X509FindType::EnumToString(findType), x509FindValue, certContexts);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "InstallCoreFxCertificate: GetCertificate [{0}] failed: {1}",
            x509FindValue,
            error);

        return error;
    }

    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "InstallCoreFxCertificate: {0} certificates found for [{1}].",
        certContexts.size(),
        x509FindValue);

    bool errorsFound = false;
    for (auto certContextIter = certContexts.begin(); certContextIter != certContexts.end(); ++certContextIter)
    {
        ErrorCode installCoreFxCertErr = LinuxCryptUtil().InstallCoreFxCertificate(StringUtility::Utf16ToUtf8(userName), *certContextIter);
        if (!installCoreFxCertErr.IsSuccess())
        {
            wstring thumbprintStr;
            Thumbprint thumbprint;
            auto error2 = thumbprint.Initialize(certContextIter->get());
            if (!error2.IsSuccess())
            {
                // Will not return on this error as thumprint is not required after that.
                TraceWarning(
                    TraceTaskCodes::Common,
                    TraceType_SecurityUtility,
                    "Can't read certificate thumbprint. ErrorCode: {0}", error2);
            }
            else
            {
                thumbprintStr = thumbprint.PrimaryToString();
            }

            errorsFound = true;
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to Install CoreFx Certificate [{0}]. Error: {1}",
                thumbprintStr,
                installCoreFxCertErr);
        }
    }

    if (errorsFound)
    {
        error = ErrorCode::FromHResult(E_FAIL);
    }

    return error;
}

ErrorCode SecurityUtility::SetCertificateAcls(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    vector<SidSPtr> const & sids,
    DWORD accessMask,
    TimeSpan const & timeout)
{
    vector<string> userNames;
    return SetCertificateAcls(x509FindValue, x509StoreName, findType, userNames, sids, accessMask, timeout);
}

Common::ErrorCode SecurityUtility::SetCertificateAcls(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    vector<string> const & userNames,
    vector<SidSPtr> const & sids,
    DWORD accessMask,
    Common::TimeSpan const & timeout)
{
    CertContexts certContexts;
    ErrorCode error = CryptoUtility::GetCertificate(X509StoreLocation::LocalMachine, x509StoreName, X509FindType::EnumToString(findType), x509FindValue, certContexts);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "SetCertificateAcls: GetCertificate failed: {0}",
            error);

        return error;
    }

    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "SetCertificateAcls: {0} certificates found.",
        certContexts.size());

    bool errorsFound = false;
    for (auto certContextIter = certContexts.begin(); certContextIter != certContexts.end(); ++certContextIter)
    {
        ErrorCode setCertAclError = SetCertificateAcls(*certContextIter, userNames, sids, accessMask, timeout);
        if (!setCertAclError.IsSuccess())
        {
            errorsFound = true;
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to set certificate acls. Error: {0}",
                setCertAclError);
        }
    }

    if (errorsFound)
    {
        error = ErrorCode::FromHResult(E_FAIL);
    }

    return error;
}

ErrorCode SecurityUtility::SetCertificateAcls(
    CertContextUPtr & certContextUPtr,
    vector<string> const & userNames,
    vector<SidSPtr> const & sids,
    DWORD accessMask,
    TimeSpan const & timeout)
{
    auto error = ErrorCode::Success();
    vector<string> files;
    map<string, DWORD> accessMap;

    string filePath = certContextUPtr.FilePath();
    LinuxCryptUtil().ReadPrivateKeyPaths(certContextUPtr, files);
    files.push_back(filePath);

    for (auto userNameIter = userNames.begin(); userNameIter != userNames.end(); userNameIter++)
    {
        string coreFxProvateKeyFilePath;
        if (LinuxCryptUtil().ReadCoreFxPrivateKeyPath(certContextUPtr, *userNameIter, coreFxProvateKeyFilePath).IsSuccess())
        {
            files.push_back(coreFxProvateKeyFilePath);
        }
    }

    sort(files.begin(), files.end());
    files.erase(unique(files.begin(), files.end()), files.end());

    for (string path : files)
    {
        accessMap[path] = accessMask;
    }

    for (string path : files)
    {
        char pathBuf[PATH_MAX];
        memcpy(pathBuf, path.c_str(), path.length() + 1);

        string dirName(dirname(pathBuf));
        while (dirName.compare("/") != 0) {
            accessMap[dirName] = FILE_TRAVERSE;
            memcpy(pathBuf, dirName.c_str(), dirName.length() + 1);
            dirName = dirname(pathBuf);
        }
    }

    bool errorsFound = false;
    for (auto it = accessMap.begin(); it != accessMap.end(); it++) {
        string path = it->first;
        unsigned int mask = it->second;
        string access;
        if (mask == FILE_TRAVERSE)
            access += "rx";
        else if (mask & FILE_ALL_ACCESS)
            access = "rw";
        else if (mask & FILE_GENERIC_READ)
            access = "r";

        for (auto iter = sids.begin(); iter != sids.end(); ++iter)
        {
            string cmd = string("setfacl ");
            string modifier;
            modifier = access.empty() ? " -x \"" : " -m \"";

            PLSID psid = (PLSID)(*iter)->PSid;
            if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_RID)
            {
                ULONG id = psid->SubAuthority[1];
                modifier += "u:" + std::to_string(id);
            }
            else if (psid->SubAuthority[0] == SECURITY_LINUX_DOMAIN_GROUP_RID)
            {
                ULONG id = psid->SubAuthority[1];
                modifier += "g:" + std::to_string(id);
            }
            if (!access.empty())
            {
                modifier += ":" + access;
            }
            modifier += "\"";
            cmd += modifier + " ";

            cmd += " " + path + " > /dev/null 2>&1";
            if (0 != system(cmd.c_str()))
            {
                TraceWarning(
                    TraceTaskCodes::Common,
                    TraceType_SecurityUtility,
                    "SetCertificateAcls: command failed: {0}",
                    cmd);

                errorsFound = true;
                break;
            }
            else
            {
                TraceInfo(
                    TraceTaskCodes::Common,
                    TraceType_SecurityUtility,
                    "SetCertificateAcls: command succeed: {0}",
                    cmd);
            }
        }
    }

    if (errorsFound)
    {
        error = ErrorCode::FromHResult(E_FAIL);
    }

    return error;
}

ErrorCode SecurityUtility::GetCertificatePrivateKeyFile(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    wstring & privateKeyFileName)
{
    vector<wstring> privateKeyFileNames;
    auto error = GetCertificatePrivateKeyFile(x509FindValue, x509StoreName, findType, privateKeyFileNames);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!privateKeyFileNames.empty())
    {
        privateKeyFileName = privateKeyFileNames[0];
    }

    return ErrorCode::Success();
}

ErrorCode SecurityUtility::GetCertificatePrivateKeyFile(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    vector<wstring> & privateKeyFileNames)
{
    CertContexts certContexts;
    ErrorCode error = CryptoUtility::GetCertificate(X509StoreLocation::LocalMachine, x509StoreName, X509FindType::EnumToString(findType), x509FindValue, certContexts);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "GetCertificatePrivateKeyFile: GetCertificate failed: {0}",
            error);

        return error;
    }

    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "GetCertificatePrivateKeyFile: {0} certificates found.",
        certContexts.size());
    
    vector<string> files;

    for (auto certContextIter = certContexts.begin(); certContextIter != certContexts.end(); ++certContextIter)
    {
        LinuxCryptUtil().ReadPrivateKeyPaths(*certContextIter, files);
    }

    sort(files.begin(), files.end());
    files.erase(unique(files.begin(), files.end()), files.end());

    for (auto fileIter = files.begin(); fileIter != files.end(); ++fileIter)
    {
        privateKeyFileNames.push_back(StringUtility::Utf8ToUtf16(*fileIter));
    }

    return error;
}

#else

ErrorCode SecurityUtility::UpdateFileAcl(
    vector<SidSPtr> const & sids,
    wstring const & fileName, 
    DWORD const accessMask,
    TimeSpan const timeout)
{
    if (!File::Exists(fileName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFileAcl: File {0} does not exist.",
            fileName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    vector<pair<SidSPtr, DWORD>> principalPermissions;
    for (auto it = sids.begin(); it != sids.end(); ++it)
    {
        principalPermissions.push_back(make_pair(*it, accessMask));
    }

    return SecurityUtility::UpdateAcl(SE_FILE_OBJECT, fileName, principalPermissions, timeout);
}

ErrorCode SecurityUtility::UpdateFolderAcl(
    Common::SidSPtr const & sid,
    std::wstring const & folderName,
    DWORD const accessMask,
    Common::TimeSpan const timeout)
{
    vector<SidSPtr> sids;
    sids.push_back(sid);
    return SecurityUtility::UpdateFolderAcl(sids, folderName, accessMask, timeout);
}

ErrorCode SecurityUtility::UpdateFolderAcl(
    vector<SidSPtr> const & sids,
    wstring const & folderName,
    DWORD const accessMask,
    TimeSpan const timeout,
    bool removeInvalidSids)
{
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    for (auto it = sids.begin(); it != sids.end(); ++it)
    {
        principalPermissions.push_back(make_pair(*it, accessMask));
    }

    return SecurityUtility::UpdateFolderAclWithStampFile(folderName, principalPermissions, removeInvalidSids, timeout);
}

ErrorCode SecurityUtility::UpdateFolderAclWithStampFile(
    wstring const & folderName,
    vector<pair<SidSPtr, DWORD>> const & principalPermissions,
    bool removeInvalidSids,
    TimeSpan const timeout)
{
    if (!Directory::Exists(folderName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl: Folder {0} does not exist.",
            folderName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    ErrorCode error(ErrorCodeValue::Success);
    TimeoutHelper timeoutHelper(timeout);
    // create lock file so that we are the only one updating the ACL
    wstring lockFile = Path::Combine(folderName, DaclLockFileName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Folder={0}, LockFileName={1}, Timeout={2}",
        folderName,
        lockFile,
        timeoutHelper.GetRemainingTime());
    {
        ExclusiveFile folderLock(lockFile);
        auto err = folderLock.Acquire(timeoutHelper.GetRemainingTime());
        if (!err.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to acquire LockFile {0}. File={1}, error={2}",
                lockFile,
                folderName,
                err);

            return ErrorCode(ErrorCodeValue::Timeout);
        }

        vector<pair<SidSPtr, DWORD>> nonExistentPrincipalPermissions;
        error = SecurityUtility::CheckACE(folderName, principalPermissions, nonExistentPrincipalPermissions);
        if (!error.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "SecurityUtility::CheckACE failed with error {0} for folderName {1}",
                error,
                folderName);
            return error;
        }

        wstring aclstampFile = Path::Combine(folderName, SecurityUtility::DaclStampFileName);

        if (!File::Exists(aclstampFile) ||
            !nonExistentPrincipalPermissions.empty())
        {
 
            File::Delete2(aclstampFile).ReadValue();
            error = SecurityUtility::UpdateAcl(SE_FILE_OBJECT, folderName, nonExistentPrincipalPermissions, timeoutHelper.GetRemainingTime(), true, L"", removeInvalidSids);

            if (!error.IsSuccess())
            {
                return ErrorCode::TraceReturn(
                        error,
                        TraceTaskCodes::Common,
                        TraceType_SecurityUtility,
                        "Failed to update ACL on folder");
            }

            error = WriteACLStampFile(aclstampFile);
        }
    }

    return error;
}

ErrorCode SecurityUtility::UpdateRegistryKeyAcl(
    Common::SidSPtr const & sid,
    std::wstring const & registryKeyName,
    DWORD const accessMask,
    Common::TimeSpan const timeout)
{
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    principalPermissions.push_back(make_pair(sid, accessMask));
    wstring lockFileName = wformatString("{0}.lock", Path::GetFileName(registryKeyName));
    TraceInfo(TraceTaskCodes::Common, TraceType_SecurityUtility, "lockFileName:{0}", lockFileName);
    return SecurityUtility::UpdateAcl(SE_REGISTRY_KEY, registryKeyName, principalPermissions, timeout, false, lockFileName);
}
Common::ErrorCode SecurityUtility::GetRegistryKeyAcl( Common::SidSPtr const & sid, std::wstring const & registryKeyName,
	DWORD & accessMask)
{
	ErrorCode error(ErrorCodeValue::NotFound);
	
	PACL pAcl = NULL;
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
	DWORD win32Error = ::GetNamedSecurityInfo(
		registryKeyName.c_str(),
		SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		&pAcl, NULL, &pSecurityDescriptor);
	if (win32Error != ERROR_SUCCESS)
	{
		return ErrorCode::TraceReturn(
			ErrorCode::FromWin32Error(win32Error),
			TraceTaskCodes::Common,
			TraceType_SecurityUtility,
			"GetRegistryKeyAcl:GetNamedSecurityInfo");
	}

	// get the size of the ACL
	ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
	aclSizeInfo.AclBytesInUse = sizeof(ACL);
	if (!::GetAclInformation(pAcl, (LPVOID)&aclSizeInfo, (DWORD)sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
	{
		::LocalFree(pSecurityDescriptor);
		return ErrorCode::TraceReturn(
			ErrorCode::FromWin32Error(),
			TraceTaskCodes::Common,
			TraceType_SecurityUtility,
			"GetRegistryKeyAcl:GetAclInformation");
	}

	PVOID pTempAce;
	for (ULONG i = 0; i < aclSizeInfo.AceCount; i++)
	{
		// get existing ace
		if (!::GetAce(pAcl, i, &pTempAce))
		{
			::LocalFree(pSecurityDescriptor);
			return ErrorCode::TraceReturn(
				ErrorCode::FromWin32Error(),
				TraceTaskCodes::Common,
				TraceType_SecurityUtility,
				"GetRegistryKeyAcl:GetAce");
		}

		if (((PACE_HEADER)pTempAce)->AceType == ACCESS_ALLOWED_ACE_TYPE)
		{
			PACCESS_ALLOWED_ACE pAllowedAce = (PACCESS_ALLOWED_ACE)pTempAce;
			if (!memcmp(&pAllowedAce->SidStart, sid->PSid, ::GetLengthSid(sid->PSid)))
			{
				accessMask = pAllowedAce->Mask;
				error = ErrorCodeValue::Success;
				break;
			}
		}
	}

	::LocalFree(pSecurityDescriptor);

	return error;
}

ErrorCode SecurityUtility::UpdateServiceAcl(
    Common::SidSPtr const & sid,
    std::wstring const & serviceName,
    DWORD const accessMask,
    Common::TimeSpan const timeout)
{
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    principalPermissions.push_back(make_pair(sid, accessMask));

    return SecurityUtility::UpdateAcl(SE_SERVICE, serviceName, principalPermissions, timeout);
}

ErrorCode SecurityUtility::UpdateAcl(
    SE_OBJECT_TYPE const objectType, 
    wstring const & objectName, 
    vector<pair<SidSPtr, DWORD>> const & principalPermissions,
    TimeSpan const timeout, 
    bool callerHoldsLock,
    wstring const & lockFile,
    bool removeInvalidSids,
    bool overwriteAcl)
{
    vector<Dacl::ACEInfo> additionalACEs;
    for (auto iter = principalPermissions.begin(); iter != principalPermissions.end(); ++iter)
    {
        auto sid = iter->first;

        additionalACEs.push_back(
            Dacl::ACEInfo(
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            iter->second,
            sid->PSid));
        additionalACEs.push_back(
            Dacl::ACEInfo(
            ACCESS_ALLOWED_ACE_TYPE,
            0,
            iter->second,
            sid->PSid));
    }

    ErrorCode error(ErrorCodeValue::Success);
    if (!callerHoldsLock)
    {
        // create lock file so that we are the only one updating the ACL  
        wstring lockFileTemp = lockFile;
        if (lockFileTemp.empty())
        {
            lockFileTemp = wformatString("{0}.lock", objectName);
        }

        TraceNoise(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Updating Object ACls. ObjectName={0}, DaclLockFileName={1}, Timeout={2}",
            objectName,
            lockFileTemp,
            timeout);

        {
            ExclusiveFile fileLock(lockFileTemp);
            if (!fileLock.Acquire(timeout).IsSuccess())
            {
                TraceWarning(
                    TraceTaskCodes::Common,
                    TraceType_SecurityUtility,
                    "Failed to acquire DaclLoclFile {0}. ObjectName={1}",
                    lockFileTemp,
                    objectName);
                return ErrorCode(ErrorCodeValue::Timeout);
            }

            if (overwriteAcl)
            {
                error = OverwriteAcl(objectType, objectName, additionalACEs);
            }
            else
            {
                error = UpdateAcl(objectType, objectName, additionalACEs, removeInvalidSids);
            }
            fileLock.Release();
        }
    }
    else
    {
        if (overwriteAcl)
        {
            error = OverwriteAcl(objectType, objectName, additionalACEs);
        }
        else
        {
            error = UpdateAcl(objectType, objectName, additionalACEs, removeInvalidSids);
        }
    }

    return error;
}

ErrorCode SecurityUtility::UpdateAcl(
    SE_OBJECT_TYPE const objectType,
    wstring const & objectName,
    vector<Dacl::ACEInfo> additionalACEs,
    bool removeInvalidSids)
{
    // get current DACL
    PACL pAcl = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD win32Error = ::GetNamedSecurityInfo(
        objectName.c_str(),
        objectType, 
        DACL_SECURITY_INFORMATION,
        NULL, 
        NULL, 
        &pAcl, NULL, &pSecurityDescriptor);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateAcl:GetNamedSecurityInfo");
    }

    DaclSPtr updatedDacl;
    try
    {
        auto error = BufferedDacl::CreateSPtr(pAcl, additionalACEs, removeInvalidSids, updatedDacl);
        ::LocalFree(pSecurityDescriptor);

        if (!error.IsSuccess()) { return error; }
    }
    catch(...)
    {
        ::LocalFree(pSecurityDescriptor);
        throw;
    }

    win32Error = ::SetNamedSecurityInfo(
        const_cast<PWCHAR>(&objectName[0]),
        objectType,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, 
        NULL, 
        NULL, 
        updatedDacl->PAcl, 
        NULL);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateAcl:SetNamedSecurityInfo");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityUtility::OverwriteAcl(
    SE_OBJECT_TYPE const objectType,
    wstring const & objectName,
    vector<Dacl::ACEInfo> additionalACEs)
{
    DaclSPtr updatedDacl;
    try
    {
        auto error = BufferedDacl::CreateSPtr(NULL, additionalACEs, updatedDacl);

        if (!error.IsSuccess()) { return error; }
    }
    catch (...)
    {
        throw;
    }

    DWORD win32Error = ::SetNamedSecurityInfo(
        const_cast<PWCHAR>(&objectName[0]),
        objectType,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        updatedDacl->PAcl,
        NULL);
    if (win32Error != ERROR_SUCCESS)
    {
         return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateAcl:SetNamedSecurityInfo");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityUtility::RemoveAcl(wstring const & resourceName, vector<SidSPtr> const & removeACLsOn, bool const disableInheritence)
{
    // get current DACL
    PACL pAcl = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD win32Error = ::GetNamedSecurityInfo(
        resourceName.c_str(), 
        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, 
        NULL, 
        NULL, 
        &pAcl, NULL, &pSecurityDescriptor);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "RemoveAcl:GetNamedSecurityInfo");
    }

    DaclSPtr updatedDacl;
    try
    {
        auto error = BufferedDacl::CreateSPtr(pAcl, removeACLsOn, updatedDacl);
        ::LocalFree(pSecurityDescriptor);

        if (!error.IsSuccess()) { return error; }
    }
    catch(...)
    {
        ::LocalFree(pSecurityDescriptor);
        throw;
    }

    SECURITY_INFORMATION secInfo = DACL_SECURITY_INFORMATION;
    if(disableInheritence)
    {
        secInfo |= PROTECTED_DACL_SECURITY_INFORMATION;
    }
    else
    {
        secInfo |= UNPROTECTED_DACL_SECURITY_INFORMATION;
    }

    win32Error = ::SetNamedSecurityInfo(
        const_cast<PWCHAR>(&resourceName[0]),
        SE_FILE_OBJECT, 
        secInfo,
        NULL, 
        NULL, 
        updatedDacl->PAcl, 
        NULL);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "RemoveAcl:SetNamedSecurityInfo");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityUtility::RemoveFolderAcl(
    Common::SidSPtr const & sid,
    std::wstring const & folderName, 
    bool const disableInheritence,    
    Common::TimeSpan const timeout)
{
    if (!Directory::Exists(folderName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "RemoveFolderAcl: File {0} does not exist.",
            folderName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    // create lock file so that we are the only one updating the ACL
    wstring lockFile = wformatString("{0}.lock", folderName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Removing ACL from folder for a Sid. Folder={0}, DaclLockFileName={1}, Timeout={2}",
        folderName,
        lockFile,
        timeout);

    ErrorCode error;
    {
        ExclusiveFile folderLock(lockFile);
        if (!folderLock.Acquire(timeout).IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to acquire DaclLoclFile {0}.",
                lockFile);
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        vector<SidSPtr> sidsToRemove;
        sidsToRemove.push_back(sid);
        error = RemoveAcl(folderName, sidsToRemove, disableInheritence);

        folderLock.Release();
    } // release dacllock

    return error;
}

ErrorCode SecurityUtility::RemoveFileAcl(
    Common::SidSPtr const & sid,
    std::wstring const & fileName,
    bool const disableInheritence,
    Common::TimeSpan const timeout)
{
    if (!File::Exists(fileName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "RemoveFileAcl: File {0} does not exist.",
            fileName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    // create lock file so that we are the only one updating the ACL
    wstring lockFile = wformatString("{0}.lock", fileName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Removing ACL from file for a Sid. File={0}, DaclLockFileName={1}, Timeout={2}",
        fileName,
        lockFile,
        timeout);

    ErrorCode error;
    {
        ExclusiveFile fileLock(lockFile);
        if (!fileLock.Acquire(timeout).IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to acquire DaclLoclFile {0}.",
                lockFile);
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        vector<SidSPtr> sidsToRemove;
        sidsToRemove.push_back(sid);
        error = RemoveAcl(fileName, sidsToRemove, disableInheritence);

        fileLock.Release();
    } // release dacllock

    return error;
}

ErrorCode SecurityUtility::RemoveInvalidFileAcls(wstring const & fileName, TimeSpan const timeout)
{
    if (!File::Exists(fileName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl: File {0} does not exist.",
            fileName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    // create lock file so that we are the only one updating the ACL
    wstring lockFile = wformatString("{0}.lock", fileName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Removing invalid file ACls. File={0}, DaclLockFileName={1}, Timeout={2}",
        fileName,
        lockFile,
        timeout);

    ErrorCode error;
    {
        ExclusiveFile fileLock(lockFile);
        if (!fileLock.Acquire(timeout).IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to acquire DaclLoclFile {0}. File={1}",
                lockFile,
                fileName);
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        error = RemoveInvalidAcl(fileName);

        fileLock.Release();
    } // release dacllock

    return error;
}


ErrorCode SecurityUtility::RemoveInvalidFolderAcls(wstring const & folderName, TimeSpan const timeout)
{
    if (!Directory::Exists(folderName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl: Folder {0} does not exist.",
            folderName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
    
    // create lock file so that we are the only one updating the ACL
    wstring lockFile = Path::Combine(folderName, DaclLockFileName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Removing invalid folder ACls. Folder={0}, DaclLockFileName={1}, Timeout={2}",
        folderName,
        lockFile,
        timeout);

    ErrorCode error;
    {
        ExclusiveFile folderLock(lockFile);
        if (!folderLock.Acquire(timeout).IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to acquire DaclLoclFile {0}. Folder={1}",
                lockFile,
                folderName);
            return ErrorCode(ErrorCodeValue::Timeout);
        }

        error = RemoveInvalidAcl(folderName);

        folderLock.Release();
    } // release dacllock

    return error;
}

ErrorCode SecurityUtility::RemoveInvalidAcl(wstring const & resourceName)
{
    // get current DACL
    PACL pAcl = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD win32Error = ::GetNamedSecurityInfo(
        resourceName.c_str(), 
        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, 
        NULL, 
        NULL, 
        &pAcl, 
        NULL, 
        &pSecurityDescriptor);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl:GetNamedSecurityInfo");
    }

    DaclSPtr updatedDacl;
    try
    {
        auto error = BufferedDacl::CreateSPtr(pAcl, true, updatedDacl);
        ::LocalFree(pSecurityDescriptor);

        if (!error.IsSuccess()) { return error; }
    }
    catch(...)
    {
        ::LocalFree(pSecurityDescriptor);
        throw;
    }

    win32Error = ::SetNamedSecurityInfo(
        const_cast<PWCHAR>(&resourceName[0]),
        SE_FILE_OBJECT, 
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, 
        NULL, 
        NULL, 
        updatedDacl->PAcl, 
        NULL);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl:SetNamedSecurityInfo");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode SecurityUtility::UpdateProcessAcl(
    ProcessHandle const & processHandle,
    Common::SidSPtr const & sid,
    ACCESS_MASK accessMask)
{
	return SecurityUtility::UpdateProcessAcl(processHandle.Value, sid, accessMask);
}

Common::ErrorCode SecurityUtility::UpdateProcessAcl(
	HANDLE const & processHandle,
	Common::SidSPtr const & sid,
	ACCESS_MASK accessMask)
{
	// get security descriptor from kernel security object
	SecurityDescriptorSPtr securityDescriptor;
	auto error = BufferedSecurityDescriptor::CreateSPtrFromKernelObject(processHandle, securityDescriptor);
	if (!error.IsSuccess()) { return error; }

	// get DACL from the security descriptor
	BOOL daclPresent;
	BOOL daclDefaulted;
	PACL dacl;
	if (!::GetSecurityDescriptorDacl(securityDescriptor->PSecurityDescriptor, &daclPresent, &dacl, &daclDefaulted))
	{
		return ErrorCode::TraceReturn(
			ErrorCode::FromWin32Error(),
			TraceTaskCodes::Common,
			TraceType_SecurityUtility,
			"UpdateProcessAcl::GetSecurityDescriptorDacl");
	}

	vector<Dacl::ACEInfo> additionalACEs;
	additionalACEs.push_back(
		Dacl::ACEInfo(
		ACCESS_ALLOWED_ACE_TYPE,
		0,
		accessMask,
		sid->PSid));

	// create updated Dacl
	DaclSPtr updatedDacl;
	error = BufferedDacl::CreateSPtr(dacl, additionalACEs, updatedDacl);
	if (!error.IsSuccess()) { return error; }

	// create new security descriptor
	SecurityDescriptorSPtr newSecurityDescriptor;
	error = BufferedSecurityDescriptor::CreateSPtr(newSecurityDescriptor);
	if (!error.IsSuccess()) { return error; }

	// set updated dacl to new security descriptor
	if (!::SetSecurityDescriptorDacl(newSecurityDescriptor->PSecurityDescriptor, TRUE, updatedDacl->PAcl, FALSE))
	{
		return ErrorCode::TraceReturn(
			ErrorCode::FromWin32Error(),
			TraceTaskCodes::Common,
			TraceType_SecurityUtility,
			"UpdateProcessAcl::SetSecurityDescriptorDacl");
	}

	// set new security descriptor on the kernel object
	if (!::SetKernelObjectSecurity(processHandle, DACL_SECURITY_INFORMATION, newSecurityDescriptor->PSecurityDescriptor))
	{
		return ErrorCode::TraceReturn(
			ErrorCode::FromWin32Error(),
			TraceTaskCodes::Common,
			TraceType_SecurityUtility,
			"UpdateProcessAcl::SetKernelObjectSecurity");
	}

	return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SecurityUtility::EnsurePrivilege(std::wstring const & privilege)
{
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Ensuring that current process has {0} privilege.",
        privilege);

    AccessTokenSPtr currentProcessToken;
    auto error = AccessToken::CreateProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, currentProcessToken);
    if (!error.IsSuccess()) { return error; }

    error = currentProcessToken->EnablePrivilege(privilege);
    if (error.IsSuccess())
    {

        bool isAllowed = false;
        error = currentProcessToken->IsAllowedPrivilege(privilege, isAllowed);
        if (error.IsSuccess())
        {
            if (isAllowed)
            {
                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                error = ErrorCode(ErrorCodeValue::OperationFailed);
            }
        }
    }

    ASSERT_IF(error.IsSuccess(), "The error must not be a success.");

    TraceError(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "The current process does not have required privilege {0}. {1}",
        privilege,
        "Modify the local security policy or run the process under an account that has this privilege.");

    return error;
}

ErrorCode SecurityUtility::GetCertificatePrivateKeyFile(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    wstring & privateKeyFileName)
{
    vector<wstring> privateKeyFileNames;
    auto error = GetCertificatePrivateKeyFile(x509FindValue, x509StoreName, findType, privateKeyFileNames);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (!privateKeyFileNames.empty())
    {
        privateKeyFileName = privateKeyFileNames[0];
    }

    return ErrorCode::Success();
}

ErrorCode SecurityUtility::GetCertificatePrivateKeyFile(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    vector<wstring> & privateKeyFileNames)
{
    if (x509FindValue.empty())
    {
        return ErrorCode::Success();
    }

    shared_ptr<X509FindValue> findValue;
    auto error = X509FindValue::Create(findType, x509FindValue, findValue);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "FindValue value '{0}'is not a valid {1}. Error: {2}",
            x509FindValue,
            findType,
            error);

        return error;
    }

    error = CryptoUtility::GetCertificatePrivateKeyFileName(
        X509StoreLocation::Enum::LocalMachine,
        x509StoreName,
        findValue,
        privateKeyFileNames);

    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Failed to get the Certificate's private key. Thumbprint:{0}. Error: {1}",
            x509FindValue,
            error);

        return error;
    }

    return ErrorCode::Success();
}

ErrorCode SecurityUtility::SetCertificateAcls(
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum findType,
    vector<SidSPtr> const & sids,
    DWORD accessMask,
    TimeSpan const & timeout)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "SetCertificateAcls. x509FindValue: {0}, x509StoreName: {1}, findType: {2}",
        x509FindValue,
        x509StoreName,
        findType);

    vector<wstring> privateKeyFiles;
    auto error = SecurityUtility::GetCertificatePrivateKeyFile(x509FindValue, x509StoreName, findType, privateKeyFiles);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Failed to get private key file. x509FindValue: {0}, x509StoreName: {1}, findType: {2}, Error {3}",
            x509FindValue,
            x509StoreName,
            findType,
            error);

        return error;
    }

    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "SetCertificateAcls. {0} Private key files found.",
        privateKeyFiles.size());

    bool errorsFound = false;
    for (auto privateKeyFile : privateKeyFiles)
    {
        auto setPrvKeyFileAclError = SetPrivateKeyFileAcls(privateKeyFile, sids, accessMask, timeout);
        if (!setPrvKeyFileAclError.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to set private key file ACL. File:{0}. Error: {1}",
                privateKeyFile,
                setPrvKeyFileAclError);
        }
    }

    if (errorsFound)
    {
        error = ErrorCode::FromHResult(E_FAIL);
    }

    return error;
}

ErrorCode SecurityUtility::SetPrivateKeyFileAcls(
    wstring const & privateKeyFile,
    vector<SidSPtr> const & sids,
    DWORD accessMask,
    TimeSpan const & timeout)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "SetPrivateKeyFileAcls. File:{0}.",
        privateKeyFile);

    auto error = SecurityUtility::RemoveInvalidFileAcls(
        privateKeyFile,
        timeout);
    error.ReadValue(); // ignore error in Inavlid removal
    vector<SidSPtr> sidsToAdd;
    for (auto iter = sids.begin(); iter != sids.end(); ++iter)
    {
        bool containsAce = false;
        error = SecurityUtility::ContainsACE(privateKeyFile, *iter, accessMask, containsAce);
        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to check if Certificate's private key contains ace. File:{0}. Error: {1}",
                privateKeyFile,
                error);
            return error;
        }
        if (!containsAce)
        {
            sidsToAdd.push_back(*iter);
        }
    }
    if (!sidsToAdd.empty())
    {
        error = SecurityUtility::UpdateFileAcl(
            sidsToAdd,
            privateKeyFile,
            accessMask,
            timeout);
        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to Acl Certificate's private key. File:{0}. Error: {1}",
                privateKeyFile,
                error);
        }
    }
    else
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Skipping to Acl Certificate's private key. File:{0} since ACEs are already present",
            privateKeyFile);
    }

    return error;
}

ErrorCode SecurityUtility::OverwriteFolderACL(
    wstring const & folderName,
    vector<pair<SidSPtr, DWORD>> const & principalPermissions,
    bool disableInheritence,
    bool ignoreInheritenceFlag,
    TimeSpan const & timeout)
{
    if (!Directory::Exists(folderName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "UpdateFolderAcl: Folder {0} does not exist.",
            folderName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    vector<Dacl::ACEInfo> additionalACEs;
    for (auto iter = principalPermissions.begin(); iter != principalPermissions.end(); ++iter)
    {
        additionalACEs.push_back(
            Dacl::ACEInfo(
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            (*iter).second,
            (*iter).first->PSid));
        additionalACEs.push_back(
            Dacl::ACEInfo(
            ACCESS_ALLOWED_ACE_TYPE,
            0,
            (*iter).second,
            (*iter).first->PSid));
    }

    ErrorCode error(ErrorCodeValue::Success);
    // create lock file so that we are the only one updating the ACL
    wstring lockFile = Path::Combine(folderName, DaclLockFileName);
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "Overwrite folder ACls. Folder={0}, DaclLockFileName={1}, Timeout={2}",
        folderName,
        lockFile,
        timeout);


    ExclusiveFile folderLock(lockFile);
    if (!folderLock.Acquire(timeout).IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Failed to acquire DaclLoclFile {0}. Folder={1}",
            lockFile,
            folderName);
        return ErrorCode(ErrorCodeValue::Timeout);
    }

    //If DACL is protected only then enable Access and inheritence 
    if (!ignoreInheritenceFlag)
    {
        bool isDACLProtected = false;
        error = SecurityUtility::IsDACLProtected(folderName, isDACLProtected);
        if (!error.IsSuccess())
        {
            return error;
        }
        if (!isDACLProtected)
        {
            TraceNoise(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Skipping overwrite folder ACLs since inheritence is already enabled and ignoreInheritenceFlag is not set");
            return error;
        }
    }
    DaclSPtr updatedDacl;
    error = BufferedDacl::CreateSPtr(NULL, additionalACEs, updatedDacl);
    if (!error.IsSuccess()) { return error; }

    SECURITY_INFORMATION secInfo = DACL_SECURITY_INFORMATION;
    if (disableInheritence)
    {
        secInfo |= PROTECTED_DACL_SECURITY_INFORMATION;
    }
    else
    {
        secInfo |= UNPROTECTED_DACL_SECURITY_INFORMATION;
    }
    DWORD win32Error = ::SetNamedSecurityInfo(
        const_cast<PWCHAR>(&folderName[0]),
        SE_FILE_OBJECT,
        secInfo,
        NULL,
        NULL,
        updatedDacl->PAcl,
        NULL);
    if (win32Error != ERROR_SUCCESS)
    {
        error = ErrorCode::FromWin32Error(win32Error);

        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "OverwriteFolderACLs failed SetNamedSecurityInfo folderName {0}, error {1}",
            folderName,
            error);
    }

    folderLock.Release();
    // release dacllock
    
    return error;
}

ErrorCode SecurityUtility::OverwriteFileAcl(
    vector<SidSPtr> const & sids,
    wstring const & fileName,
    DWORD const accessMask,
    TimeSpan const timeout)
{
    if (!File::Exists(fileName))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "OverwriteFileAcl: File {0} does not exist.",
            fileName);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    vector<pair<SidSPtr, DWORD>> principalPermissions;
    for (auto it = sids.begin(); it != sids.end(); ++it)
    {
        principalPermissions.push_back(make_pair(*it, accessMask));
    }

    return SecurityUtility::UpdateAcl(SE_FILE_OBJECT, fileName, principalPermissions, timeout, false, L"", false, true);
}

Common::ErrorCode SecurityUtility::IsDACLProtected(wstring const & resourceName, bool & isProtected)
{
    PACL pAcl;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD win32Error = ::GetNamedSecurityInfo(
        resourceName.c_str(),
        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        &pAcl, NULL, &pSecurityDescriptor);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "IsDACLProtected:GetNamedSecurityInfo");
    }

    SECURITY_DESCRIPTOR_CONTROL sdControl;
    DWORD revision;
    if (!GetSecurityDescriptorControl(pSecurityDescriptor, &sdControl, &revision))
    {
        win32Error = ::GetLastError();
    }
    ::LocalFree(pSecurityDescriptor);
 
    if (sdControl & SE_DACL_PROTECTED)
    { 
        isProtected = true; 
    }
    else
    {
        isProtected = false;
    }
    return ErrorCode::TraceReturn(
        ErrorCode::FromWin32Error(win32Error),
        TraceTaskCodes::Common,
        TraceType_SecurityUtility,
        "IsDACLProtected:GetSecurityDescriptorControl");
}

ErrorCode SecurityUtility::ContainsACE(wstring const & folderName, SidSPtr const & sid, DWORD accessMask, bool & containsAce)
{
    vector<pair<SidSPtr, DWORD>> principalPermissions;
    vector<pair<SidSPtr, DWORD>> nonExistentPrincipalPermissions;
    principalPermissions.push_back(make_pair(sid, accessMask));
    containsAce = false;
    auto error = SecurityUtility::CheckACE(folderName, principalPermissions, nonExistentPrincipalPermissions);
    if (!error.IsSuccess())
    {
        return error;
    }

    containsAce = (nonExistentPrincipalPermissions.size() == 0);
    return error;
}

ErrorCode  SecurityUtility::CheckACE(wstring const & folderName,
    vector<pair<SidSPtr, DWORD>> const & principalPermissions,
    vector<pair<SidSPtr, DWORD>> & nonExistentPrincipalPermissions)
{
    PACL pAcl = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD win32Error = ::GetNamedSecurityInfo(
        folderName.c_str(),
        SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        &pAcl,
        NULL,
        &pSecurityDescriptor);
    if (win32Error != ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "ContainsACE:GetNamedSecurityInfo");
    }

    // get information on the current ACLs
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    if (!::GetAclInformation(pAcl, (LPVOID)&aclSizeInfo, (DWORD)sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "GetAclInformation");
    }

    PVOID pTempAce;
    wstring targetSid;
    nonExistentPrincipalPermissions.clear();
    for (auto it = principalPermissions.begin(); it != principalPermissions.end(); ++it)
    {
        auto error = it->first->ToString(targetSid);
        if (!error.IsSuccess())
        {
            return ErrorCode::TraceReturn(
                error,
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "ContainsACE:Failed to convert sidstring to sidsptr");
        }

        bool containsAce = false;
        for (ULONG i = 0; i < aclSizeInfo.AceCount; i++)
        {
            // get existing ace
            if (!::GetAce(pAcl, i, &pTempAce))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_SecurityUtility,
                    "GetAce");
            }
            PSID pTempSid;
            auto aceType = ((PACE_HEADER)pTempAce)->AceType;
            if ((aceType == ACCESS_ALLOWED_ACE_TYPE))
            {
                pTempSid = (PSID)&((PACCESS_ALLOWED_ACE)pTempAce)->SidStart;
                wstring sidString;
                if (Sid::ToString(pTempSid, sidString).IsSuccess() &&
                    StringUtility::AreEqualCaseInsensitive(sidString, targetSid) &&
                    ((PACCESS_ALLOWED_ACE)pTempAce)->Mask == it->second)
                {
                    containsAce = true;
                    break;
                }
            }
        }

        if (!containsAce)
        {
            nonExistentPrincipalPermissions.push_back(make_pair(it->first, it->second));
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode SecurityUtility::WriteACLStampFile(wstring const & filelocation)
{
    try
    {
        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(filelocation);
        if (!error.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Failed to open ACL timestamp file {0}, error={1}",
                filelocation,
                error);
            return error;
        }

        wstring text;
        text.append(L"ACL success");
        std::string result;
        StringUtility::UnicodeToAnsi(text, result);
        fileWriter << result;
    }
    catch (std::exception const& e)
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Failed to generate ACL timestamp file {0}, exception ={1}",
            filelocation,
            e.what());
        return ErrorCodeValue::OperationFailed;
    }
    return ErrorCodeValue::Success;
}

Common::ErrorCode SecurityUtility::IsCurrentUserAdmin(bool & isAdminUser)
{
    isAdminUser = false;
    Common::SidUPtr localAdminsGroupSidUptr;
    ErrorCode error = Common::BufferedSid::CreateUPtr(WELL_KNOWN_SID_TYPE::WinBuiltinAdministratorsSid, localAdminsGroupSidUptr);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_SecurityUtility,
            "Creation of buffered SID failed with {0}",
            error);

        return error;
    }
    else
    {
        BOOL isAdmin = TRUE;
        if (::CheckTokenMembership(NULL, localAdminsGroupSidUptr->PSid, &isAdmin))
        {
            isAdminUser = isAdmin == TRUE;
        }
        else
        {
            error = ErrorCode::FromWin32Error();
            TraceError(
                TraceTaskCodes::Common,
                TraceType_SecurityUtility,
                "Checking of token membership failed with {0}",
                error);

            return error;
        }
        return error;
    }
}

#endif
