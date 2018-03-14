// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CertificateAclingManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        public Common::IConfigUpdateSink,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        class CertificateConfig;

        DENY_COPY(CertificateAclingManager)

    public:
        CertificateAclingManager(Common::ComponentRoot const & root);
        virtual ~CertificateAclingManager();

        virtual bool OnUpdate(std::wstring const & section, std::wstring const & key);
        virtual bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted);
        virtual const std::wstring & GetTraceId() const;

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        void TimerCallback(Common::TimerSPtr const & timer);
        Common::ErrorCode AclCertificates();
        Common::ErrorCode SetCertificateAcls(
            std::wstring const & findValue,
            std::wstring const & findType,
            std::wstring const & storeName,
            std::vector<SidSPtr> const & targetSids);
        Common::ErrorCode SetCertificateAcls(
            std::wstring const & findValue,
            Common::X509FindType::Enum findType,
            std::wstring const & storeName,
            std::vector<SidSPtr> const & targetSids);
        bool TrySetCertificateAcls(
            std::wstring const & certificateName,
            std::wstring const & findValue,
            std::wstring const & findType,
            std::wstring const & storeName,
            std::vector<SidSPtr> const & targetSids,
            bool& errorsFound);
        Common::ErrorCode AclConfiguredCertificates(std::vector<Common::SidSPtr> const & targetSids);
        Common::ErrorCode AclAnonymousCertificate(std::vector<Common::SidSPtr> const & targetSids);
        void ScheduleCertificateAcling();
        void StopTimer();
        bool TryReadConfigKey(std::wstring const & section, std::wstring const & key, std::wstring & value);

    private:
        Common::Config config_;
        std::wstring traceId_;
        Common::TimerSPtr timer_;
        Common::RwLock lock_;
        std::vector<CertificateConfig> certificateConfigs_;

    private:
        class CertificateConfig
        {
        public:
            CertificateConfig(
                std::wstring const & title,
                std::wstring const & sectionName,
                std::wstring const & storeNameKeyName,
                std::wstring const & findTypeKeyName,
                std::wstring const & findValueKeyName);
        public:
            std::wstring GetTitle();
            std::wstring GetSectionName();
            std::wstring GetStoreNameKeyName();
            std::wstring GetFindTypeKeyName();
            std::wstring GetFindValueKeyName();

        private:
            std::wstring title_;
            std::wstring sectionName_;
            std::wstring storeNameKeyName_;
            std::wstring findTypeKeyName_;
            std::wstring findValueKeyName_;
        };
    };
}
