// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define METADATATABLE_Tag 'ttDM'

using namespace Data::TStore;
using namespace Data::Utilities;

ULONG MyHashFunction(const UINT& Val)
{
    return ~Val;
}

MetadataTable::MetadataTable(
   __in ULONG32 size,
   __in LONG64 checkpointLSN,
   __in HashFunctionType func)
    :checkpointLSN_(checkpointLSN),
    isClosed_(false),
    referenceCount_(1) // Start with reference count 1 to account for adding ref on creation time.
{
    ULONG32Comparer::SPtr comparerSPtr;
    NTSTATUS status = ULONG32Comparer::Create(this->GetThisAllocator(), comparerSPtr);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    IComparer<ULONG32>::SPtr keyComparerSPtr = comparerSPtr.DownCast<IComparer<ULONG32>>();
    
    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr dictionarySPtr = nullptr;
    status = ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::Create(
        ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::DefaultConcurrencyLevel,
        size,
        func,
        keyComparerSPtr,
        GetThisAllocator(),
        dictionarySPtr);
    SetConstructorStatus(status);

    tableSPtr_ = static_cast<IDictionary<ULONG32, FileMetadata::SPtr> *>(dictionarySPtr.RawPtr());
}

MetadataTable::~MetadataTable()
{
}

bool MetadataTable::Test_IsCheckpointLSNExpected = false;

NTSTATUS
MetadataTable::Create(
    __in KAllocator& allocator,
    __out MetadataTable::SPtr& result,
    __in LONG64 checkpointLSN)
{
    NTSTATUS status;
    SPtr output = _new(METADATATABLE_Tag, allocator) MetadataTable(32, checkpointLSN, MyHashFunction);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

void MetadataTable::AddReference()
{
    auto refCount = InterlockedIncrement64(&referenceCount_);
    ASSERT_IFNOT(refCount >= 2, "Unbalanced AddRef-ReleaseRef detected!");
}

bool MetadataTable::TryAddReference()
{
    LONG64 refCount = 0;

    // Spin instead of locking since contention is low
    do
    {
        refCount = referenceCount_; // TODO: volatile read?
        if (refCount == 0)
        {
            return false;
        }
    } while (InterlockedCompareExchange64(&referenceCount_, refCount + 1, refCount) != refCount);

    return true;
}

ktl::Awaitable<void> MetadataTable::ReleaseReferenceAsync()
{
    KShared$ApiEntry();

    auto refCount = InterlockedDecrement64(&referenceCount_);
    ASSERT_IFNOT(refCount >= 0, "Unbalanced AddRef-ReleaseRef detected!");

    if (refCount == 0)
    {
       co_await CloseAsync();
    }
}

ktl::Awaitable<void> MetadataTable::CloseAsync()
{
    KShared$ApiEntry();

    try
    {
        if (!isClosed_)
        {
            if (tableSPtr_ != nullptr)
            {
                KSharedPtr<IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>> tableEnumerator = tableSPtr_->GetEnumerator();
                while (tableEnumerator->MoveNext())
                {
                    KeyValuePair<ULONG32, FileMetadata::SPtr> entry = tableEnumerator->Current();
                    auto key = entry.Key;
                    FileMetadata::SPtr fileMetadata;
                    bool result = tableSPtr_->TryGetValue(key, fileMetadata);
                    KInvariant(result);
                    co_await fileMetadata->ReleaseReferenceAsync();
                }
            }

            isClosed_ = true;
        }
    }
    catch (ktl::Exception const & e)
    {
        // We are asserting here because this can be executed in a KTL Task in some cases
        KDynStringA stackString(this->GetThisAllocator());
        Diagnostics::GetExceptionStackTrace(e, stackString);
        ASSERT_IFNOT(
            false,
            "UnexpectedException: Message: {2} Code:{4}\nStack: {3}",
            L"MetadataTable.CloseAsync",
            ToStringLiteral(stackString),
            e.GetStatus());
    }
}

void MetadataTable::TestMarkAsClosed()
{
   isClosed_ = true;
}
