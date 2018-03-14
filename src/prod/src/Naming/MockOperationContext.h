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
    class MockOperationContext :
        public IFabricAsyncOperationContext,
        public Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComReplicatedOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext)

    public:
        MockOperationContext() { }

        BOOLEAN STDMETHODCALLTYPE IsCompleted() { Common::Assert::CodingError("IsCompleted() not implemented"); }
        BOOLEAN STDMETHODCALLTYPE CompletedSynchronously() { return TRUE; }
        HRESULT STDMETHODCALLTYPE get_Callback(IFabricAsyncOperationCallback **) { Common::Assert::CodingError("CallbackState() not implemented"); }
        HRESULT STDMETHODCALLTYPE Cancel() { Common::Assert::CodingError("Cancel() not implemented"); }
    };
}
