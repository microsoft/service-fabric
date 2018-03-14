// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        //
        // The Test Com StateProvider2Factory implements the IFabricStateProvider2Factory interface.
        // Its purpose is to convert the internal KTL TestStateProviderFactory in to COM based interface.
        // This is used to create state providers (IStateProvider2).
        //
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
            static Common::ComPointer<IFabricStateProvider2Factory> Create(
                __in Data::StateManager::IStateProvider2Factory & innerFactory,
                __in KAllocator & allocator);

            // Returns a pointer to a HANDLE (PHANDLE).
            // HANDLE is IStateProvider2*.
            STDMETHOD_(HRESULT, Create)(
                /* [in] */ FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
                /* [retval][out] */ HANDLE * stateProvider) override;

        private:
            TestComStateProvider2Factory(__in Data::StateManager::IStateProvider2Factory & innerFactory);

        private:
            Data::StateManager::IStateProvider2Factory::SPtr innerFactory_;
        };
    }
}
