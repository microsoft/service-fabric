// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Interop
    {
        class StateProviderFactory :
            public KObject<StateProviderFactory>,
            public KShared<StateProviderFactory>,
            public Data::StateManager::IStateProvider2Factory
        {
            K_FORCE_SHARED(StateProviderFactory)
            K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

        public:
            NTSTATUS Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;
            
            static NTSTATUS Create(
                __in KAllocator& allocator,
                __out StateProviderFactory::SPtr& factory);

        private:

            StateProviderFactory(
                __in Data::StateManager::IStateProvider2Factory* storeFactory,
                __in Data::StateManager::IStateProvider2Factory* rcqFactory);

            Data::StateManager::IStateProvider2Factory::SPtr storeFactory_;
            Data::StateManager::IStateProvider2Factory::SPtr rcqFactory_;
        };
    }
}
