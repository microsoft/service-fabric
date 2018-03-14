// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGOperationDataAsyncEnumerator : 
        public IFabricOperationDataStream, 
        public Common::ComUnknownBase
    {
        DENY_COPY(SGOperationDataAsyncEnumerator);

        BEGIN_COM_INTERFACE_LIST(SGOperationDataAsyncEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
        END_COM_INTERFACE_LIST()

    public:
        //
        // Constructor/Destructor.
        //
        SGOperationDataAsyncEnumerator(
            Common::ComPointer<IFabricOperationData> && operationData,
            bool failBegin,
            bool failEnd,
            bool empty);
        virtual ~SGOperationDataAsyncEnumerator(void);
        //
        // IFabricOperationDataStream methods.
        //
        virtual HRESULT STDMETHODCALLTYPE BeginGetNext(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        virtual HRESULT STDMETHODCALLTYPE EndGetNext( 
            __in IFabricAsyncOperationContext* context,
            __out IFabricOperationData** operationData
            );

    protected:
        Common::ComPointer<IFabricOperationData> emptyData_;
        Common::ComPointer<IFabricOperationData> operationData_;
        BOOLEAN empty_;
        BOOLEAN failBegin_;
        BOOLEAN failEnd_;

    private:
        static Common::StringLiteral const TraceSource;
    };
};
