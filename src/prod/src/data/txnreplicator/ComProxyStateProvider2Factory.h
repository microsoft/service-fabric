// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The state manager uses the user state provider 2 factory to create state providers.
    // This proxyclass converts the KTL based interface to com based API's.
    //
    class ComProxyStateProvider2Factory final
        : public KObject<ComProxyStateProvider2Factory>
        , public KShared<ComProxyStateProvider2Factory>
        , public Data::StateManager::IStateProvider2Factory
    {
        K_FORCE_SHARED(ComProxyStateProvider2Factory)
        K_SHARED_INTERFACE_IMP(IStateProvider2Factory)

    public:
        static NTSTATUS Create(
            __in Common::ComPointer<IFabricStateProvider2Factory> & factory,
            __in KAllocator & allocator,
            __out SPtr & comProxyStateProviderFactory);

    public:
        NTSTATUS Create(
            __in Data::StateManager::FactoryArguments const & factoryArguments,
            __out IStateProvider2::SPtr & stateProvider) noexcept override;

    private:
        ComProxyStateProvider2Factory(__in Common::ComPointer<IFabricStateProvider2Factory> & v1StateReplicator);

    private:
        Common::ComPointer<IFabricStateProvider2Factory> comFactory_;
        Common::ScopedHeap heap_;
    };
}
