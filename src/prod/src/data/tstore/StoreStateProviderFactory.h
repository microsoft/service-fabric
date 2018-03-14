// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STORE_STATE_PROVIDER_FACTORY_TAG 'fpSS'

namespace Data
{
    namespace TStore
    {
        class StoreStateProviderFactory :
            public KObject<StoreStateProviderFactory>,
            public KShared<StoreStateProviderFactory>,
            public Data::StateManager::IStateProvider2Factory
        {
            K_FORCE_SHARED(StoreStateProviderFactory)
            K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

        public:

            NTSTATUS Create(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

            static SPtr CreateLongStringUTF8Factory(__in KAllocator& allocator);

            static SPtr CreateIntIntFactory(__in KAllocator& allocator);

            static SPtr CreateStringUTF8BufferFactory(__in KAllocator& allocator);

            static SPtr CreateNullableStringUTF8BufferFactory(__in KAllocator& allocator);

            static SPtr CreateStringUTF16BufferFactory(__in KAllocator& allocator);

            static SPtr CreateLongBufferFactory(__in KAllocator& allocator);

            static SPtr CreateBufferBufferFactory(__in KAllocator& allocator);

            enum FactoryDataType : int
            {
                IntInt = 0,

                LongStringUTF8 = 1,

                StringUTF8Buffer = 2,

                StringUTF16Buffer = 3,

                LongBuffer = 4,

                BufferBuffer = 5,

                NullableStringUTF8Buffer = 6
            };

        private:

            void CreateIntKeyIntValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateLongKeyStringUTF8ValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateStringUTF8KeyBufferValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateNullableStringUTF8KeyBufferValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateStringUTF16KeyBufferValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateLongKeyBufferValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            void CreateBufferKeyBufferValueStore(
                __in Data::StateManager::FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider);

            StoreStateProviderFactory(__in FactoryDataType type);

            FactoryDataType type_;
        };
    }
}
