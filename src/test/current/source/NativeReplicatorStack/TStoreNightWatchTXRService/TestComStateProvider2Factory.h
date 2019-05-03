// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TStoreNightWatchTXRService
{
    class TestComStateProvider2Factory final
        : public KObject<TestComStateProvider2Factory>
        , public KShared<TestComStateProvider2Factory>
        , public IFabricStateProvider2Factory
    {
        K_FORCE_SHARED(TestComStateProvider2Factory)

        K_BEGIN_COM_INTERFACE_LIST(TestComStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider2Factory, IFabricStateProvider2Factory)
        K_END_COM_INTERFACE_LIST()

    public:
        static Common::ComPointer<IFabricStateProvider2Factory> Create(__in KAllocator & allocator);

        virtual HRESULT STDMETHODCALLTYPE Create(
            /* [in] */ const FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS *factoryArguments,
            /* [retval][out] */ void **stateProvider);

    private:
        Data::TStore::StoreStateProviderFactory::SPtr innerFactory_;
    };
}
