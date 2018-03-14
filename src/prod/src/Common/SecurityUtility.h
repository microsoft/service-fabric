// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct SecurityUtility
    {
    public:
        static Common::ErrorCode UpdateFolderAcl(
            Common::SidSPtr const & sid,
            std::wstring const & folderName,
            DWORD const accessMask,
            Common::TimeSpan const timeout);

        static Common::ErrorCode UpdateFolderAcl(
            std::vector<Common::SidSPtr> const & sids,
            std::wstring const & folderName,
            DWORD const accessMask,
            Common::TimeSpan const timeout,
            bool removeInvalidSids = false);

        static Common::ErrorCode UpdateFolderAclWithStampFile(
            std::wstring const & folderName,
            std::vector<std::pair<Common::SidSPtr, DWORD>> const & principalPermissions,
            bool removeInvalidSids,
            Common::TimeSpan const timeout);

        static Common::ErrorCode UpdateFileAcl(
            std::vector<Common::SidSPtr> const & sids,
            std::wstring const & fileName, 
            DWORD const accessMask,
            Common::TimeSpan const timeout);

        static Common::ErrorCode UpdateProcessAcl(
            ProcessHandle const & processHandle,
            Common::SidSPtr const & sid,
            ACCESS_MASK accessMask);

		static Common::ErrorCode UpdateProcessAcl(
			HANDLE const & processHandle,
			Common::SidSPtr const & sid,
			ACCESS_MASK accessMask);

        static Common::ErrorCode RemoveFolderAcl(
            Common::SidSPtr const & sid,
            std::wstring const & folderName,
            bool const disableInheritence,            
            Common::TimeSpan const timeout);

        static Common::ErrorCode RemoveFileAcl(
            Common::SidSPtr const & sid,
            std::wstring const & folderName,
            bool const disableInheritence,
            Common::TimeSpan const timeout);

        static Common::ErrorCode RemoveInvalidFolderAcls(
            std::wstring const & folderName,
            Common::TimeSpan const timeout);

        static Common::ErrorCode RemoveInvalidFileAcls(
            std::wstring const & fileName,
            Common::TimeSpan const timeout);

        static Common::ErrorCode EnsurePrivilege(
            std::wstring const & privilege);

        static Common::ErrorCode GetCertificatePrivateKeyFile(
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            X509FindType::Enum findType,
            std::wstring & privateKeyFileName);

        static Common::ErrorCode GetCertificatePrivateKeyFile(
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            X509FindType::Enum findType,
            vector<std::wstring> & privateKeyFileNames);

        static Common::ErrorCode SetCertificateAcls(
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            X509FindType::Enum findType,
            std::vector<SidSPtr> const & sids,
            DWORD accessMask,
            Common::TimeSpan const & timeout);

        static Common::ErrorCode OverwriteFolderACL(
            std::wstring const & folderName,
            std::vector<std::pair<SidSPtr, DWORD>> const & principalPermissions,
            bool disableInheritence,
            bool ignoreInheritence, 
            Common::TimeSpan const & timeout);

       static Common::ErrorCode OverwriteFileAcl(
            vector<SidSPtr> const & sids,
            wstring const & fileName,
            DWORD const accessMask,
            TimeSpan const timeout);

        static Common::ErrorCode UpdateRegistryKeyAcl(
            Common::SidSPtr const & sid,
            std::wstring const & registryKeyName,
            DWORD const accessMask,
            Common::TimeSpan const timeout);

		static Common::ErrorCode GetRegistryKeyAcl(
			Common::SidSPtr const & sid,
			std::wstring const & registryKeyName,
			DWORD & accessMask);

        static Common::ErrorCode UpdateServiceAcl(
            Common::SidSPtr const & sid,
            std::wstring const & serviceName,
            DWORD const accessMask,
            Common::TimeSpan const timeout);

        static Common::ErrorCode ContainsACE(
            std::wstring const & folderName,
            Common::SidSPtr const & sid,
            DWORD accessMask,
            bool & containsAce);

        static Common::ErrorCode CheckACE(
            std::wstring const & folderName,
            std::vector<std::pair<Common::SidSPtr, DWORD>> const & principalPermissions,
            std::vector<std::pair<Common::SidSPtr, DWORD>> & nonExistentPrincipalPermissions);

        static Common::ErrorCode IsCurrentUserAdmin(bool & isAdminUser);

#ifdef PLATFORM_UNIX
        static int GetPwByUid(uid_t uid, passwd & pwd);
        static int GetGrByGid(gid_t gid, group & grp);
        static Common::ErrorCode SetCertificateAcls(
            CertContextUPtr & certContextUPtr,
			std::vector<std::string> const & userNames,
            vector<SidSPtr> const & sids,
            DWORD accessMask,
            TimeSpan const & timeout);
        static Common::ErrorCode InstallCoreFxCertificate(
			std::wstring const & userName,
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            X509FindType::Enum findType);
		static Common::ErrorCode SetCertificateAcls(
			std::wstring const & x509FindValue,
			std::wstring const & x509StoreName,
			X509FindType::Enum findType,
			std::vector<std::string> const & userNames, //List of usernames to acl certificates in there corefx directory
			std::vector<SidSPtr> const & sids,
			DWORD accessMask,
			Common::TimeSpan const & timeout);
#endif

    private:

#ifndef PLATFORM_UNIX
        static Common::ErrorCode UpdateAcl(
            SE_OBJECT_TYPE const objectType, 
            std::wstring const & resourceName, 
            std::vector<std::pair<SidSPtr, DWORD>> const & principalPermissions,
            Common::TimeSpan const timeout,
            bool callerHoldsLock = false,
            std::wstring const & lockFile = L"",
            bool removeInvalidSids = false,
            bool overwriteAcl = false);

        static Common::ErrorCode UpdateAcl(
            SE_OBJECT_TYPE const objectType,
            std::wstring const & resourceName,
            std::vector<Dacl::ACEInfo> additionalACEs,
            bool removeInvalidSids = false);

       static Common::ErrorCode OverwriteAcl(
            SE_OBJECT_TYPE const objectType,
            wstring const & objectName,
            vector<Dacl::ACEInfo> additionalACEs);

       static Common::ErrorCode SetPrivateKeyFileAcls(
           wstring const & privateKeyFile,
           vector<SidSPtr> const & sids,
           DWORD accessMask,
           TimeSpan const & timeout);
#endif

        static Common::ErrorCode RemoveAcl(std::wstring const & resourceName, std::vector<Common::SidSPtr> const & removeACLsOn, bool const disableInheritence);
        static Common::ErrorCode RemoveInvalidAcl(std::wstring const & resourceName);
        static Common::ErrorCode IsDACLProtected(std::wstring const & resourceName, bool & isProtected);
        static Common::ErrorCode WriteACLStampFile(std::wstring const & fileLocation);
        static std::wstring const DaclLockFileName;
        static std::wstring const DaclStampFileName;
    };
}
