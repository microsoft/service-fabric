// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricAsyncOperationCallback :
        public KShared<FabricAsyncOperationCallback>,
        public IFabricAsyncOperationCallback
    {
        K_FORCE_SHARED(FabricAsyncOperationCallback);

        K_BEGIN_COM_INTERFACE_LIST(FabricAsyncOperationCallback)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricAsyncOperationCallback)
            K_COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out FabricAsyncOperationCallback::SPtr& spCallback,
            __in KAllocator& allocator
            );

    public:
        virtual void Invoke(
            __in IFabricAsyncOperationContext *context
            );

    public:
        bool Wait(
            __in ULONG timeoutInMilliseconds
            );

    private:
        Common::ManualResetEvent _event;
    };
}
