// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Log;

LogicalLogKIoBufferStream::LogicalLogKIoBufferStream() {}

LogicalLogKIoBufferStream::LogicalLogKIoBufferStream(
    __in KIoBuffer& underlyingIoBuffer,
    __in ULONG initialOffset) :
    KIoBufferStream(underlyingIoBuffer, initialOffset),
    underlyingIoBuffer_(&underlyingIoBuffer)
{
}

BOOLEAN
LogicalLogKIoBufferStream::Reuse(
    __in KIoBuffer& underlyingIoBuffer,
    __in ULONG initialOffset)
{
    underlyingIoBuffer_ = &underlyingIoBuffer;
    return KIoBufferStream::Reuse(underlyingIoBuffer, initialOffset);
}
