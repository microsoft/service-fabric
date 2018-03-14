// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

    class ComStateProviderFactory final
        : public KObject<ComStateProviderFactory>
        , public KShared<ComStateProviderFactory>
        , public IFabricStateProvider2Factory
    {
        K_FORCE_SHARED(ComStateProviderFactory)

            K_BEGIN_COM_INTERFACE_LIST(ComStateProviderFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider2Factory, IFabricStateProvider2Factory)
            K_END_COM_INTERFACE_LIST()

    public:
		static HRESULT ComStateProviderFactory::Create(
			__in KAllocator & allocator,
			__out Common::ComPointer<IFabricStateProvider2Factory>& result);

        // Returns a pointer to a HANDLE (PHANDLE).
        // HANDLE is IStateProvider2*.
        STDMETHOD_(HRESULT, Create)(
            /* [in] */ FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
            /* [retval][out] */ HANDLE * stateProvider) override;

    private:
        ComStateProviderFactory(__in Data::StateManager::IStateProvider2Factory & innerFactory);

    private:
        Data::StateManager::IStateProvider2Factory::SPtr innerFactory_;
    };
