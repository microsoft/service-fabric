// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {8F1ED49F-8BBC-445F-A70E-AB39D6300C16}
    static const GUID CLSID_ComContainerActivatorServiceAgent =
    { 0x8f1ed49f, 0x8bbc, 0x445f,{ 0xa7, 0xe, 0xab, 0x39, 0xd6, 0x30, 0xc, 0x16 } };

    class ComContainerActivatorServiceAgent :
        public IFabricContainerActivatorServiceAgent2,
        private Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComContainerActivatorServiceAgent)

        BEGIN_COM_INTERFACE_LIST(ComContainerActivatorServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricContainerActivatorServiceAgent2)
            COM_INTERFACE_ITEM(IID_IFabricContainerActivatorServiceAgent, IFabricContainerActivatorServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricContainerActivatorServiceAgent2, IFabricContainerActivatorServiceAgent2)
            COM_INTERFACE_ITEM(CLSID_ComContainerActivatorServiceAgent, ComContainerActivatorServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComContainerActivatorServiceAgent(IContainerActivatorServiceAgentPtr const & impl);
        virtual ~ComContainerActivatorServiceAgent();

        IContainerActivatorServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricContainerActivationManagerAgent methods
        // 
        HRESULT STDMETHODCALLTYPE ProcessContainerEvents(
            /* [in] */ FABRIC_CONTAINER_EVENT_NOTIFICATION *notifiction);

        HRESULT STDMETHODCALLTYPE RegisterContainerActivatorService(
            /* [in] */ IFabricContainerActivatorService * activatorService);

        // 
        // IFabricContainerActivationManagerAgent2 methods
        // 
        HRESULT STDMETHODCALLTYPE RegisterContainerActivatorService(
            /* [in] */ IFabricContainerActivatorService2 * activatorService);

    private:
        IContainerActivatorServiceAgentPtr impl_;
    };
}