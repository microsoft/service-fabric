//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once
#define RCQ_STATE_PROVIDER_FACTORY_TAG 'fpCQ'

namespace Data
{
    namespace Collections 
    {
        class RCQStateProviderFactory :
            public KObject<RCQStateProviderFactory>,
            public KShared<RCQStateProviderFactory>,
            public Data::StateManager::IStateProvider2Factory
        {
            K_FORCE_SHARED(RCQStateProviderFactory)
            K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

        public:

            NTSTATUS Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

            static SPtr CreateBufferFactory(__in KAllocator& allocator);

            enum FactoryDataType : int
            {
                Buffer = 1,
            };

        private:

            void CreateBufferValueRCQ(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            RCQStateProviderFactory(__in FactoryDataType type);

            FactoryDataType type_;
        };
    }
}
