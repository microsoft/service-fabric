// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class PortAclRef :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>

    {
    public:
        PortAclRef(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::vector<std::wstring> const & prefixes);

        ~PortAclRef();

        __declspec(property(get=get_Port)) UINT Port;
        UINT get_Port() const { return port_; }

        __declspec(property(get = get_CurrentRefCount)) UINT CurrentRefCount;
        UINT get_CurrentRefCount() const { return refCount_; }

        __declspec(property(get = get_IsClosed)) bool IsClosed;
        bool get_IsClosed() const { return closed_; }

        __declspec(property(get = get_PrincipalSid)) std::wstring const & PrincipalSid;
        std::wstring const & get_PrincipalSid() const { return principalSid_; }


        Common::ErrorCode ConfigurePortAclsIfNeeded(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::wstring const & sddl,
            HttpPortAclCallback const & callback);

        Common::ErrorCode RemovePortAclsIfNeeded(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::wstring const & sddl,
            HttpPortAclCallback const & callback);

        Common::ErrorCode RemoveAllForNode(
            std::wstring const & nodeId,
            std::wstring const & sddl,
            HttpPortAclCallback const & callback);

    private:
        std::wstring const principalSid_;
        std::map<std::wstring, std::set<std::wstring>, Common::IsLessCaseInsensitiveComparer<std::wstring>> ownerIds_;
        UINT port_;
        UINT refCount_;
        bool isHttps_;
        std::vector<std::wstring> prefixes_;
       
        bool closed_;
        Common::RwLock lock_;
    };    
}
