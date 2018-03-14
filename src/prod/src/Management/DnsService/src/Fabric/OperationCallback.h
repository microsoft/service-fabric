// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    typedef KDelegate<void(__in IFabricAsyncOperationContext* pResolveContext)> FabricAsyncOperationCallback;

    class OperationCallback :
        public KShared<OperationCallback>,
        public IFabricAsyncOperationCallback
    {
        K_FORCE_SHARED(OperationCallback);

        K_BEGIN_COM_INTERFACE_LIST(OperationCallback)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricAsyncOperationCallback)
            K_COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out OperationCallback::SPtr& spCallback,
            __in KAllocator& allocator,
            __in FabricAsyncOperationCallback callback
            );

    private:
        OperationCallback(
            __in FabricAsyncOperationCallback callback
        );

    public:
        virtual void Invoke(
            __in_opt IFabricAsyncOperationContext *context
            );

    private:
        FabricAsyncOperationCallback _callback;
    };
}
