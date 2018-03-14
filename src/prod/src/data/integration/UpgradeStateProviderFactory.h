// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Integration
    {
        class UpgradeStateProviderFactory :
            public KObject<UpgradeStateProviderFactory>,
            public KShared<UpgradeStateProviderFactory>,
            public Data::StateManager::IStateProvider2Factory
        {
            K_FORCE_SHARED(UpgradeStateProviderFactory)
            K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

        public:
            static SPtr Create(
                __in KAllocator& allocator);

            static SPtr Create(
                __in KAllocator& allocator,
                __in bool isBackCompatTest);

        public:
            NTSTATUS Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

        private:
            UpgradeStateProviderFactory(__in bool isBackCompatTest);

        private:
            bool const isBackCompatTest_;

        private:
            // Note: For now the BackCompatible BackupAndRestore test only covers the <int, int> type.
            Data::TStore::StoreStateProviderFactory::SPtr innerFactory_;
        };
    }
}
