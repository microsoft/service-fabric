// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

SharedException::SharedException()
    : exception_(STATUS_SUCCESS)
    , stackTrace_(GetThisAllocator())
{
}

SharedException::SharedException(__in ktl::Exception const & exception)
    : exception_(exception)
    , stackTrace_(GetThisAllocator())
{
    exception_.ToString(stackTrace_,"\r\n");
}

SharedException::~SharedException()
{
}

SharedException::CSPtr SharedException::Create(
    __in Exception const & exception,
    __in KAllocator & allocator)
{
    SharedException * ex = _new(SHAREDEXCEPTION_TAG, allocator)SharedException(exception);
    ASSERT_IF(ex == nullptr, "OOM while allocating SharedException");

    return SharedException::CSPtr(ex);
}

SharedException::CSPtr SharedException::Create(__in KAllocator & allocator)
{
    SharedException * ex = _new(SHAREDEXCEPTION_TAG, allocator)SharedException();
    ASSERT_IF(ex == nullptr, "OOM while allocating SharedException");

    return SharedException::CSPtr(ex);
}
