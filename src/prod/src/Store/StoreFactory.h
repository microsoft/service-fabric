// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{ 
    class StoreFactory : public IStoreFactory
    {
    public:
        Common::ErrorCode CreateLocalStore(
            StoreFactoryParameters const &,
            __out ILocalStoreSPtr &) override;

        Common::ErrorCode CreateLocalStore(
            StoreFactoryParameters const &,
            __out ILocalStoreSPtr &,
            __out ServiceModel::HealthReport &) override;

    private:

        Common::ErrorCode CreateEseLocalStore(StoreFactoryParameters const &, __out ILocalStoreSPtr &);
        void CreateTSLocalStore(StoreFactoryParameters const &, __out ILocalStoreSPtr &);

        Common::ErrorCode HandleEseMigration(StoreFactoryParameters const &);
        Common::ErrorCode HandleTStoreMigration(StoreFactoryParameters const &);

        ServiceModel::HealthReport CreateHealthyReport(
            StoreFactoryParameters const &, 
            ProviderKind::Enum);

        ServiceModel::HealthReport CreateUnhealthyReport(
            StoreFactoryParameters const &,
            ProviderKind::Enum,
            std::wstring const & details);

        void CreateHealthReportAttributes(
            StoreFactoryParameters const &, 
            __out ServiceModel::EntityHealthInformationSPtr &,
            __out ServiceModel::AttributeList &);

        ServiceModel::HealthReport CreateHealthReport(
            StoreFactoryParameters const &, 
            ProviderKind::Enum);
    };
}
