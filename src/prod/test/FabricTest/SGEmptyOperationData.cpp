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
SGEmptyOperationData::SGEmptyOperationData()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGEmptyOperationData::SGEmptyOperationData ({0}) - ctor",
        this);
}

SGEmptyOperationData::~SGEmptyOperationData()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGEmptyOperationData::~SGEmptyOperationData ({0}) - dtor",
        this);
}

//
// IFabricOperationData methods
//
HRESULT STDMETHODCALLTYPE SGEmptyOperationData::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    if (NULL == count || NULL == buffers)
    {
        return E_POINTER;
    }

    TestSession::WriteNoise(
        TraceSource, 
        "SGEmptyOperationData::GetData ({0})",
        this);

    *count = 0;
    *buffers = NULL;

    return S_OK;
}

StringLiteral const SGEmptyOperationData::TraceSource("FabricTest.ServiceGroup.SGEmptyOperationData");
