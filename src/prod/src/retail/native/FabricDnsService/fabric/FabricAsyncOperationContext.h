// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricAsyncOperationContext :
        public KShared<FabricAsyncOperationContext>,
        public IFabricAsyncOperationContext
    {
        K_FORCE_SHARED(FabricAsyncOperationContext);

        K_BEGIN_COM_INTERFACE_LIST(FabricAsyncOperationContext)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricAsyncOperationContext)
            K_COM_INTERFACE_ITEM(IID_IFabricAsyncOperationContext, IFabricAsyncOperationContext)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out FabricAsyncOperationContext::SPtr& spContext,
            __in KAllocator& allocator,
            __in IFabricAsyncOperationCallback* callback
            );

    private:
        FabricAsyncOperationContext(
            __in IFabricAsyncOperationCallback* callback
            );

    public:
        virtual BOOLEAN IsCompleted() override
        {
            return _fIsCompleted;
        }

        virtual BOOLEAN CompletedSynchronously() override
        {
            return _fCompletedSync;
        }

        virtual HRESULT get_Callback(
            __out IFabricAsyncOperationCallback **callback
            ) override;

        virtual HRESULT Cancel(void) override;

    public:
        void SetCompleted(__in BOOLEAN fCompleted) { _fIsCompleted = fCompleted; }

        void SetCompletedSync(__in BOOLEAN fCompleted) { _fCompletedSync = fCompleted; }

    private:
        BOOLEAN _fIsCompleted;
        BOOLEAN _fCompletedSync;
        ComPointer<IFabricAsyncOperationCallback> _spCallback;
    };
}
