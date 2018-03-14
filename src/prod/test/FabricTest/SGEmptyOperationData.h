// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGEmptyOperationData : 
        public IFabricOperationData,
        public Common::ComUnknownBase
    {
        DENY_COPY(SGEmptyOperationData);

        BEGIN_COM_INTERFACE_LIST(SGEmptyOperationData)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()
    public:
        //
        // Constructor/Destructor.
        //
        SGEmptyOperationData();
        virtual ~SGEmptyOperationData();
        //
        // IFabricOperationData methods.
        //
        virtual HRESULT STDMETHODCALLTYPE GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);

    private:
        static Common::StringLiteral const TraceSource;
    };
};
