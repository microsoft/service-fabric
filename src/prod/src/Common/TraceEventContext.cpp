// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

void* TraceEventContext::GetExternalBuffer(size_t size)
{
    void* result;

    if (size <= externalAvailable_)
    {
        result = externalBuffer_ + externalUsed_;
        externalUsed_ += size;
        externalAvailable_ -= size;
    }
    else
    {
        size_t newSize = externalUsed_ + size + BufferSize;
        char* newBuffer = new char[newSize];
        // TODO: handle allocation failure

        if (externalUsed_)
        {
            memcpy_s(newBuffer, newSize, externalBuffer_, externalUsed_);
            delete[] externalBuffer_;
        }

        externalBuffer_ = newBuffer;
        result = externalBuffer_ + externalUsed_;
        externalUsed_ += size;
        externalAvailable_ = BufferSize;
    }

    return result;
}
