// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class UnreliableTransportConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(UnreliableTransportConfig, "UnreliableTransport")

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UnreliableTransport", MaxAllowedDelayInSeconds, Common::TimeSpan::FromSeconds(40), Common::ConfigEntryUpgradePolicy::Static)

    public:

        bool AddSpecification(UnreliableTransportSpecificationUPtr && spec);

        bool AddSpecification(std::wstring const & name, std::wstring const & data);

        bool RemoveSpecification(std::wstring const & name);

        Common::TimeSpan GetDelay(std::wstring const & source, std::wstring const & destination, std::wstring const & action);
        
        Common::TimeSpan GetDelay(std::wstring const & source, std::wstring const & destination, Transport::Message const & message);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        virtual void Initialize();

        Common::ErrorCode StartMonitoring(std::wstring const & workFolder, std::wstring const & nodeId);

        Common::ErrorCode Test_StopMonitoring(std::wstring const & nodeId);

        static Common::ErrorCode ReadFromINI(std::wstring const & fileName, std::vector<std::wstring> & parameters, bool shouldLockFile = true);

        static Common::ErrorCode ReadFromINI(std::wstring const & fileName, std::vector<std::pair<std::wstring, std::wstring> > & parameters, bool shouldLockFile = true);

        static Common::ErrorCode WriteToINI(std::wstring const & fileName, std::vector<std::pair<std::wstring, std::wstring> > const & parameters, bool shouldLockFile = true);

        bool AddSpecificationNoSourceNode(std::wstring const & nodeId, std::wstring const & name, std::wstring const & data);

        bool IsDisabled();

        static Common::GlobalWString SettingsFileName;

    private:
        typedef std::vector<UnreliableTransportSpecificationUPtr> SpecTable;

        void LoadConfiguration(Common::Config & config);

        void LoadConfigurationFromINI(std::wstring const & fileName, std::wstring const & nodeId, bool shouldLockFile = true);

        static Common::ErrorCode ReadTextFile(std::wstring const & fileName, std::string & text);

        static Common::ErrorCode WriteTextFile(std::wstring const & fileName, std::vector<std::pair<std::wstring, std::wstring> > const & parameters);

        void ClearNodeIdSpecifications(std::wstring const & nodeId);

        void UpdateIsUnreliableTransportEnabled();

        SpecTable specifications_;
        Common::RwLock specsTableLock_;

        static BOOL CALLBACK InitSingleton(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static UnreliableTransportConfig* Singleton;

        bool isDisabledUnreliableTransport_;
        std::map<std::wstring, Common::FileChangeMonitor2SPtr> nodeIdFileChangeMonitor;     // dictionary of file monitors where the key is the nodeid
        Common::RwLock fileChangeMonitorDictionaryLock_;
    };
}
