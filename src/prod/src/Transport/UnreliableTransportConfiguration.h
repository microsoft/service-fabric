// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class UnreliableTransportConfiguration : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(UnreliableTransportConfiguration, "UnreliableTransport")

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UnreliableTransport", MaxAllowedDelayInSeconds, Common::TimeSpan::FromSeconds(40), Common::ConfigEntryUpgradePolicy::Dynamic)

    public:
        bool AddSpecification(UnreliableTransportSpecificationUPtr && spec);

        bool AddSpecification(std::wstring const & name, std::wstring const & data);

        bool RemoveSpecification(std::wstring const & name);

        Common::TimeSpan GetDelay(std::wstring const & source, std::wstring const & destination, std::wstring const & action);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        virtual void Initialize();

    private:
        typedef std::vector<UnreliableTransportSpecificationUPtr> SpecTable;

        void LoadConfiguration(Common::Config & config);

        SpecTable specifications_;
        Common::RwLock specsTableLock_;

        static BOOL CALLBACK InitSingleton(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static UnreliableTransportConfiguration* Singleton;
    };
}
