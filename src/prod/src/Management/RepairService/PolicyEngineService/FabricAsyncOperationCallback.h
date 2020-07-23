// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class FabricAsyncOperationCallback  :
        public IFabricAsyncOperationCallback,
        private Common::ComUnknownBase
    {
        DENY_COPY(FabricAsyncOperationCallback);

        COM_INTERFACE_LIST1(FabricAsyncOperationCallback, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

    public:
        FabricAsyncOperationCallback();
        ~FabricAsyncOperationCallback();

        /*IFabricAsyncOperationCallback members*/
        virtual void STDMETHODCALLTYPE Invoke( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
    };
}
