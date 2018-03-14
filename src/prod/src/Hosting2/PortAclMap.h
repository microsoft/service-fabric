// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class PortAclMap :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
         DENY_COPY(PortAclMap)

    public:
        PortAclMap();
        ~PortAclMap();

        Common::ErrorCode GetOrAdd(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::vector<std::wstring> const & prefixes,
            __out PortAclRefSPtr & portAclRef);

        Common::ErrorCode Find(
            std::wstring const & principalSid,
            UINT port,
            __out PortAclRefSPtr & portAclRef) const;
        Common::ErrorCode Remove(UINT port);

        Common::ErrorCode GetOrAddCertRef(
            UINT port,
            std::wstring const & principalSid,
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            Common::X509FindType::Enum x509FindType,
            __out PortCertRefSPtr & portCertRef);

        Common::ErrorCode GetCertRef(
            UINT port,
            std::wstring const & principalSid,
            std::wstring const & x509FindValue,
            std::wstring const & x509StoreName,
            Common::X509FindType::Enum x509FindType,
            __out PortCertRefSPtr & portCertRef);

        Common::ErrorCode FindCertRef(
            UINT port,
            __out PortCertRefSPtr & portCertRef) const;

        Common::ErrorCode RemoveCertRef(UINT port);

        std::vector<PortAclRefSPtr> Close();
        
        void GetAllPortRefs(
            std::vector<PortAclRefSPtr> & portAcls,
            std::vector<PortCertRefSPtr> & portCertRefs);


    private:
        mutable Common::RwLock lock_;
        mutable Common::RwLock certLock_;

        bool closed_;

        std::map<UINT, PortAclRefSPtr> map_;
        std::map < UINT, PortCertRefSPtr> certRefMap_;
    };
}
