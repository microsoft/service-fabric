// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

InterlockedBufferPoolItem::InterlockedBufferPoolItem()
: pNext_(nullptr)
{
    // Do not initialize m_buffer because its initial contents don't matter.
}

InterlockedBufferPoolItem::~InterlockedBufferPoolItem()
{
    // TODO: should we zero?
    // If we ever have buffer overrun bugs, protect against information disclosure for data in previous requests.
    ::ZeroMemory(&buffer_, sizeof(buffer_));
}
