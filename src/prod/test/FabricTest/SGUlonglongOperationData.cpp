// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace FabricTest;

//
// Constructor/Destructor
//
SGUlonglongOperationData::SGUlonglongOperationData(
    ULONGLONG value)
    : value1_(value/3)
    , value2_(value%3)
{
    replicaBuffer_[0].BufferSize = sizeof(ULONGLONG);
    replicaBuffer_[0].Buffer = reinterpret_cast<BYTE*>(&value1_);
    replicaBuffer_[1].BufferSize = sizeof(ULONGLONG);
    replicaBuffer_[1].Buffer = reinterpret_cast<BYTE*>(&value2_);

    TestSession::WriteNoise(
        TraceSource, 
        "SGUlonglongOperationData::SGUlonglongOperationData ({0}) - ctor - value({1})",
        this,
        value1_*3 + value2_);
}

SGUlonglongOperationData::~SGUlonglongOperationData()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGUlonglongOperationData::~SGUlonglongOperationData ({0}) - dtor - value({1})",
        this,
        value1_*3 + value2_);
}

//
// IFabricOperationData methods
//
HRESULT STDMETHODCALLTYPE SGUlonglongOperationData::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    if (NULL == count || NULL == buffers)
    {
        return E_POINTER;
    }

    TestSession::WriteNoise(
        TraceSource, 
        "SGUlonglongOperationData::GetData ({0}) - value({1})",
        this,
        value1_*3 + value2_);

    *count = 2;
    *buffers = &replicaBuffer_[0];

    return S_OK;
}
StringLiteral const SGUlonglongOperationData::TraceSource("FabricTest.ServiceGroup.SGUlonglongOperationData");
