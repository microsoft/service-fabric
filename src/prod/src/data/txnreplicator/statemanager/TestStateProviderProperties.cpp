// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data; 
using namespace Data::Utilities;
using namespace StateManagerTests;

NTSTATUS TestStateProviderProperties::Create(
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(TEST_STATE_PROVIDER_PROPERTY_TAG, allocator) TestStateProviderProperties();
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

void TestStateProviderProperties::Write(__in Data::Utilities::BinaryWriter & writer)
{
    // 'TestStateProviderInfo' - BlockHandle
    writer.Write(TestStateProviderPropertyName_);
    Data::Utilities::VarInt::Write(writer, Data::Utilities::BlockHandle::SerializedSize());
    testStateProviderInfoHandle_->Write(writer);
}

void TestStateProviderProperties::ReadProperty(
    __in Data::Utilities::BinaryReader & reader,
    __in KStringView const & property,
    __in ULONG32)
{
    if (property.Compare(TestStateProviderPropertyName_) == 0)
    {
        testStateProviderInfoHandle_ = Data::Utilities::BlockHandle::Read(reader, GetThisAllocator());
    }
}

TestStateProviderProperties::TestStateProviderProperties()
    : testStateProviderInfoHandle_(nullptr)
{
}

TestStateProviderProperties::~TestStateProviderProperties()
{
}
