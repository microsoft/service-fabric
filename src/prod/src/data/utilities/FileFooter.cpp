// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define FILEFOOTER_TAG 'ffFF'

using namespace Data::Utilities;

NTSTATUS FileFooter::Create(
    __in BlockHandle & propertiesHandle,
    __in ULONG32 version,
    __in KAllocator & allocator,
    __out FileFooter::SPtr & result) noexcept
{
    result = _new(FILEFOOTER_TAG, allocator) FileFooter(propertiesHandle, version);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

ULONG32 FileFooter::SerializedSize()
{
    return BlockHandle::SerializedSize() + sizeof(ULONG32);
}

FileFooter::SPtr FileFooter::Read(
    __in BinaryReader & binaryReader,
    __in BlockHandle const & handle,
    __in KAllocator & allocator)
{
    UNREFERENCED_PARAMETER(handle);

    BlockHandle::SPtr propertiesHandle = BlockHandle::Read(binaryReader, allocator);
    ULONG32 version;
    binaryReader.Read(version);

    FileFooter::SPtr result;
    NTSTATUS status = FileFooter::Create(*propertiesHandle, version, allocator, result);
    THROW_ON_FAILURE(status);

    return result;
}

void FileFooter::Write(__in BinaryWriter & binaryWriter)
{
    PropertiesHandle_->Write(binaryWriter);
    binaryWriter.Write(Version_);
}

FileFooter::FileFooter(
    __in BlockHandle & propertiesHandle,
    __in ULONG32 version)
    : PropertiesHandle_(&propertiesHandle)
    , Version_(version)
{
}

FileFooter::~FileFooter()
{
}
