// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{    
    class SMBShareUtility
    {
    public:
        static GlobalWString WindowsFabricSMBShareRemark;        
        
        static GlobalWString NullSessionSharesRegistryValue;
        static GlobalWString LanmanServerParametersRegistryPath;

        static GlobalWString LsaRegistryPath;
        static GlobalWString EveryoneIncludesAnonymousRegistryValue;

        static ErrorCode EnsureServerServiceIsRunning();

        static ErrorCode CreateShare(
            std::wstring const & localPath, 
            std::wstring const & shareName, 
            SecurityDescriptorSPtr const & securityDescriptor);
        
        static ErrorCode DeleteShare(std::wstring const & shareName);
        
        static std::set<std::wstring> DeleteOrphanShares();

        static ErrorCode EnableAnonymousAccess(
            std::wstring const & localPath,
            std::wstring const & shareName,
            DWORD accessMask,
            SidSPtr const & anonymousSid,
			TimeSpan const & timeout);

        static ErrorCode DisableAnonymousAccess(std::wstring const & localPath, std::wstring const & shareName, TimeSpan const & timeout);
    };
}
