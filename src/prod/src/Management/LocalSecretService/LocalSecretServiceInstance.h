//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------
#pragma once
namespace Management
{
    namespace LocalSecretService
    {
        class LocalSecretServiceInstance :
            public IFabricStatelessServiceInstance,
            private ComUnknownBase
        {
            DENY_COPY(LocalSecretServiceInstance)

            BEGIN_COM_INTERFACE_LIST(LocalSecretServiceInstance)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceInstance)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
            END_COM_INTERFACE_LIST()

        public:
            LocalSecretServiceInstance();

            ~LocalSecretServiceInstance();

            // IFabricStatelessServiceInstance Impl.
            virtual HRESULT BeginOpen(
                __in IFabricStatelessServicePartition* partition,
                __in IFabricAsyncOperationCallback* callback,
                __out IFabricAsyncOperationContext** context
            ) override;

            virtual HRESULT EndOpen(
                __in IFabricAsyncOperationContext* context,
                __out IFabricStringResult** serviceAddress
            ) override;

            virtual HRESULT BeginClose(
                __in IFabricAsyncOperationCallback* callback,
                __out IFabricAsyncOperationContext** context
            ) override;

            virtual HRESULT EndClose(
                __in IFabricAsyncOperationContext* context
            ) override;

            virtual void Abort(void) override;
        };
    }
}
