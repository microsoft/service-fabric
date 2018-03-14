// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class PortCertRef :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>

    {
    public:
        PortCertRef(
            std::wstring const & principalSid,
            UINT port,
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            Common::X509FindType::Enum x509FindType);

        ~PortCertRef();

        __declspec(property(get=get_Port)) UINT Port;
        UINT get_Port() const { return port_; }

        __declspec(property(get = get_CurrentRefCount)) UINT CurrentRefCount;
        UINT get_CurrentRefCount() const { return refCount_; }

        __declspec(property(get = get_IsClosed)) bool IsClosed;
        bool get_IsClosed() const { return closed_; }

        __declspec(property(get = get_PrincipalSid)) std::wstring const & PrincipalSid;
        std::wstring const & get_PrincipalSid() const { return principalSid_; }

        __declspec(property(get = get_CertFindValue)) std::wstring const & CertFindValue;
        std::wstring const & get_CertFindValue() const { return x509FindValue_; }

        __declspec(property(get = get_CertStoreName)) std::wstring const & CertStoreName;
        std::wstring const & get_CertStoreName() const { return x509StoreName_; }

        __declspec(property(get = get_CertFindType)) Common::X509FindType::Enum CertFindType;
        Common::X509FindType::Enum get_CertFindType() const { return x509FindType_; }


        Common::ErrorCode ConfigurePortCertIfNeeded(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            PortCertificateBindingCallback const & callback);

        Common::ErrorCode RemovePortCertIfNeeded(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            PortCertificateBindingCallback const & callback);

        Common::ErrorCode RemoveAllForNode(
            std::wstring const & nodeId,
            PortCertificateBindingCallback const & certCallback);

    private:
        std::wstring const principalSid_;
        std::map<std::wstring, std::set<std::wstring>, Common::IsLessCaseInsensitiveComparer<std::wstring>> ownerIds_;
         UINT port_;
        UINT refCount_;
        
        std::wstring const x509FindValue_;
        std::wstring const x509StoreName_;
        Common::X509FindType::Enum x509FindType_;

        bool closed_;
        Common::RwLock lock_;
    };    
}
