// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class FabricAsyncOperationContext :
        public IFabricAsyncOperationContext,
        private Common::ComUnknownBase
    {
        DENY_COPY(FabricAsyncOperationContext)

        COM_INTERFACE_LIST1(FabricAsyncOperationContext, IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext);

    public:
        FabricAsyncOperationContext();
        ~FabricAsyncOperationContext();
        void Initialize(
            IFabricAsyncOperationCallback* callback);
        
        /*IFabricAsyncOperationContext members*/
        BOOLEAN STDMETHODCALLTYPE IsCompleted( void);
        BOOLEAN STDMETHODCALLTYPE CompletedSynchronously( void);
        HRESULT STDMETHODCALLTYPE get_Callback( 
            /* [retval][out] */ IFabricAsyncOperationCallback **callback);
        HRESULT STDMETHODCALLTYPE Cancel( void);

    private:
        Common::ComPointer<IFabricAsyncOperationCallback> callbackCPtr_;
    };
}
