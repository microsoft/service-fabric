// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGComStatelessService : 
        public IFabricStatelessServiceInstance,
        public Common::ComUnknownBase
    {
        DENY_COPY(SGComStatelessService);

        BEGIN_COM_INTERFACE_LIST(SGComStatelessService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
        END_COM_INTERFACE_LIST()

    public:
        //
        // Constructor/Destructor.
        //
        explicit SGComStatelessService(SGStatelessService & service);
        virtual ~SGComStatelessService();

        //
        // IFabricStatelessServiceInstance methods. 
        //
        HRESULT STDMETHODCALLTYPE BeginOpen( 
            IFabricStatelessServicePartition *servicePartition,
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceEndpoint);
        
        HRESULT STDMETHODCALLTYPE BeginClose( 
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            IFabricAsyncOperationContext *context);

        void STDMETHODCALLTYPE Abort();
    private:
        SGStatelessService & service_;
        Common::ComponentRootSPtr root_;

        static Common::StringLiteral const TraceSource;
    };
};
