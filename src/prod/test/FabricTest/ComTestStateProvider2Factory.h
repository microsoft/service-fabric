// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    //
    // The Test Com StateProvider2Factory implements the IFabricStateProvider2Factory interface.
    // Its purpose is to convert the internal KTL TestStateProviderFactory in to COM based interface.
    // This is used to create state providers (IStateProvider2).
    //
    class ComTestStateProvider2Factory final
        : public KObject<ComTestStateProvider2Factory>
        , public KShared<ComTestStateProvider2Factory>
        , public IFabricStateProvider2Factory
    {
        K_FORCE_SHARED(ComTestStateProvider2Factory)

        K_BEGIN_COM_INTERFACE_LIST(ComTestStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider2Factory)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider2Factory, IFabricStateProvider2Factory)
        K_END_COM_INTERFACE_LIST()

        public:
            static Common::ComPointer<IFabricStateProvider2Factory> Create(
                __in std::wstring const & nodeId,
                __in std::wstring const & serviceName,
                __in KAllocator & allocator);

            // Returns a pointer to a HANDLE (PHANDLE).
            // HANDLE is IStateProvider2*.
            STDMETHOD_(HRESULT, Create)(
                /* [in] */ FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
                /* [retval][out] */ HANDLE * stateProvider) override;

        private:

            ComTestStateProvider2Factory(
                __in std::wstring const & nodeId,
                __in std::wstring const & serviceName);

            Common::ComPointer<IFabricStateProvider2Factory> innerComFactory_;
            std::wstring const nodeId_;
            std::wstring const serviceName_;
    };
}
