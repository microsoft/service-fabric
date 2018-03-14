// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HttpEndpointSecurityProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(HttpEndpointSecurityProvider)

    public:
        HttpEndpointSecurityProvider(
            Common::ComponentRoot const & root);
        virtual ~HttpEndpointSecurityProvider();

        Common::ErrorCode ConfigurePortAcls(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::wstring const & additionalPrefix = std::wstring(),
            std::wstring const & servicePackageId = std::wstring(),
            std::wstring const & nodeId = std::wstring(),
            bool isExplicitPort = false);

        Common::ErrorCode CleanupPortAcls(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::wstring const & additionalPrefix = std::wstring(),
            std::wstring const & servicePackageId = std::wstring(),
            std::wstring const & nodeId = std::wstring(),
            bool isExplicitPort = false);

        Common::ErrorCode ConfigurePortCertificate(
            UINT port,
            std::wstring const& sslCertificateFindValue,
            std::wstring const& sslCertStoreName,
            Common::X509FindType::Enum sslCertificateFindType);

        Common::ErrorCode CleanupPortCertificate(
            UINT port,
            std::wstring const& sslCertificateFindValue,
            std::wstring const& sslCertStoreName,
            Common::X509FindType::Enum sslCertificateFindType);

        Common::ErrorCode ConfigurePortCertificate(
            UINT port,
            std::wstring const & principalSid,
            bool isSharedPort,
            bool removeConfiguration,
            std::wstring const& sslCertificateFindValue,
            std::wstring const& sslCertStoreName,
            Common::X509FindType::Enum sslCertificateFindType,
            std::wstring const & servicePackageId,
            std::wstring const & nodeId);

        Common::ErrorCode CertificateConfigurationHelper(
            bool setup,
            UINT port,
            std::wstring const& sslCertificateFindValue,
            std::wstring const& sslCertStoreName,
            Common::X509FindType::Enum sslCertificateFindType);

       void CleanupAllForNode(std::wstring const & nodeId);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        typedef std::shared_ptr<Common::ResourceHolder<PVOID>> HttpEngineSPtr;

    private:
        Common::ErrorCode InitializeHttpEngine();
        void CleanupHttpEngine();

#if !defined(PLATFORM_UNIX)
        Common::ErrorCode RegisterEndpointWithHttpEngine(
            std::wstring const & sddl, 
            UINT port,
            bool isHttps,
            std::vector<std::wstring> const & prefixes);

        Common::ErrorCode UnregisterEndpointWithHttpEngine(
            std::wstring const & sddl, 
            UINT port,
            bool isHttps,
            std::vector<std::wstring> const & prefixes);

        Common::ErrorCode SetupSharedPortAcls(
            std::wstring const & servicePackageId,
            std::wstring const & nodeId,
            std::wstring const & sddl,
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::vector<std::wstring> const & prefixes);
#endif

       Common::ErrorCode CleanupSharedPortAcls(
           std::wstring const & servicePackageId,
           std::wstring const & nodeId,
           std::wstring const & sddl,
           std::wstring const & principalSid,
           UINT port,
           bool isHttps,
           std::vector<std::wstring> const & prefixes);

        static Common::ErrorCode GetSddl(
            std::wstring const & principalSid, 
            __out std::wstring & sddl);

        Common::ErrorCode CleanupSharedPortCert(PortCertRefSPtr const & portCertRef);
        Common::ErrorCode CleanupSharedPortAcl(PortAclRefSPtr const & portAclRef);

        static std::wstring GetHttpEndpoint(UINT port, std::wstring const & prefix, bool isHttps = false);

    private:
        HttpEngineSPtr httpEngine_;
        Common::RwLock httpEngineLock_;
        PortAclMapUPtr portAclMap_;
    };
}
