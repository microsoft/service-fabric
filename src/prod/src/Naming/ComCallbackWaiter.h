// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// *************
// For CITs only
// *************

namespace Naming
{
    class ComCallbackWaiter : 
        public Common::AutoResetEvent,
        public IFabricAsyncOperationCallback,
        public Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComCallbackWaiter,
            IID_IFabricAsyncOperationCallback,
            IFabricAsyncOperationCallback);

    public:
        ComCallbackWaiter() : Common::AutoResetEvent(false) { }

        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext *) 
        { 
            this->Set(); 
        }
    };
}
