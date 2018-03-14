// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGUlonglongOperationData : 
        public IFabricOperationData,
        public Common::ComUnknownBase
    {
        DENY_COPY(SGUlonglongOperationData);

        BEGIN_COM_INTERFACE_LIST(SGUlonglongOperationData)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()
    public:
        //
        // Constructor/Destructor.
        //
        SGUlonglongOperationData(ULONGLONG value);
        virtual ~SGUlonglongOperationData(void);
        //
        // IFabricOperationData methods.
        //
        virtual HRESULT STDMETHODCALLTYPE GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);

    private:
        ULONGLONG value1_;
        ULONGLONG value2_;
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_[2];

        static Common::StringLiteral const TraceSource;
    };
};
