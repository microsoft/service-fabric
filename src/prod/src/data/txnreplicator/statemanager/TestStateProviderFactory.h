// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestStateProviderFactory :
        public KObject<TestStateProviderFactory>,
        public KShared<TestStateProviderFactory>,
        public Data::StateManager::IStateProvider2Factory
    {
        K_FORCE_SHARED(TestStateProviderFactory)
        K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

    public:
        static SPtr Create(__in KAllocator & allocator);

    public:
        __declspec(property(get = get_FaultAPI, put = put_FaultAPI)) FaultStateProviderType::FaultAPI FaultAPI;
        FaultStateProviderType::FaultAPI get_FaultAPI() const { return faultAPI_; };
        void put_FaultAPI(__in FaultStateProviderType::FaultAPI faultAPI) { faultAPI_ = faultAPI; }

    public:
        NTSTATUS Create(
            __in Data::StateManager::FactoryArguments const & factoryArguments,
            __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept override;

    private:
        FaultStateProviderType::FaultAPI faultAPI_;
    };
}
