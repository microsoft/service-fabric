// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define MERGEHELPER_TAG 'rpHM'

using namespace Data::TStore;

ULONG MyHashFunction(ULONG32& Val)
{
    return ~Val;
}

MergeHelper::MergeHelper()
    : fileTypeToMergeList_(nullptr),
    mergePolicy_(MergePolicy::All)
{
    NTSTATUS status = FileCountMergeConfiguration::Create(this->GetThisAllocator(), fileCountMergeConfigurationSPtr_);
    Diagnostics::Validate(status);
    
    ULONG32Comparer::SPtr comparerSPtr;
    status = ULONG32Comparer::Create(this->GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    IComparer<ULONG32>::SPtr keyComparerSPtr = static_cast<IComparer<ULONG32> *>(comparerSPtr.RawPtr());
    ASSERT_IFNOT(keyComparerSPtr != nullptr, "file id comparer should not be null");

    status = Dictionary<ULONG32, KSharedArray<ULONG32>::SPtr>::Create(
        32, 
        ULONG32Comparer::HashFunction, 
        *keyComparerSPtr, 
        GetThisAllocator(), fileTypeToMergeList_);
    Diagnostics::Validate(status);

    mergeFilesCountThreshold_ = DefaultMergeFilesCountThreshold;
    percentageOfInvalidEntriesPerFile_ = DefaultPercentageOfInvalidEntriesPerFile;
    percentageOfDeletedEntriesPerFile_ = DefaultPercentageOfDeletedEntriesPerFile;

    // File count merge policy
    NumberOfInvalidEntries = 0;

    // Size on disk merge policy
    SizeOnDiskThreshold = DefaultSizeForSizeOnDiskPolicy;
}

MergeHelper::~MergeHelper()
{
}

NTSTATUS
MergeHelper::Create(
    __in KAllocator& allocator,
    __out MergeHelper::SPtr& result)
{
    NTSTATUS status;

    SPtr output = _new(MERGEHELPER_TAG, allocator) MergeHelper();

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

ktl::Awaitable<bool> MergeHelper::ShouldMerge(
    __in MetadataTable& mergeTable, 
    __out KSharedArray<ULONG32>::SPtr& mergeList)
{
    if (mergeTable.Table->Count < MergeFilesCountThreshold)
    {
        mergeList = nullptr;
        co_return false;
    }

    if (IsMergePolicyEnabled(MergePolicy::FileCount))
    {
        bool hasVal = co_await ShouldMergeDueToFileCountPolicy(mergeTable, mergeList);

        if (hasVal)
        {
            co_return true;
        }
    }

    if (IsMergePolicyEnabled(MergePolicy::InvalidEntries) || IsMergePolicyEnabled(MergePolicy::DeletedEntries))
    {
        mergeList = GetMergeList(mergeTable);

        auto count = mergeList->Count();
        auto threshold = static_cast<ULONG>(MergeFilesCountThreshold);
        if (count >= threshold)
        {
            co_return true;
        }
    }

    if (IsMergePolicyEnabled(MergePolicy::SizeOnDisk))
    {
        bool shouldMerge = co_await ShouldMergeForSizeOnDiskPolicy(mergeTable);

        if (shouldMerge)
        {
            mergeList = _new(MERGEHELPER_TAG, GetThisAllocator()) KSharedArray<ULONG32>();
            auto mergeTableEnumerator = mergeTable.Table->GetEnumerator();
            while (mergeTableEnumerator->MoveNext())
            {
                auto item = mergeTableEnumerator->Current();
                mergeList->Append(item.Key);
            }
            co_return true;
        }
    }

    mergeList = nullptr;
    co_return false;
}

KSharedArray<ULONG32>::SPtr MergeHelper::GetMergeList(__in MetadataTable& mergeTable)
{
    KSharedArray<ULONG32>::SPtr mergeFileIds = _new(MERGEHELPER_TAG, GetThisAllocator()) KSharedArray<ULONG32>();

    bool invalidEntriesEnabled = IsMergePolicyEnabled(MergePolicy::InvalidEntries);
    bool deletedEntriesEnabled = IsMergePolicyEnabled(MergePolicy::DeletedEntries);

    auto mergeTableEnumerator = mergeTable.Table->GetEnumerator();
    while (mergeTableEnumerator->MoveNext())
    {
        auto item = mergeTableEnumerator->Current();
        if (invalidEntriesEnabled && IsFileQualifiedForInvalidEntriesMergePolicy(item))
        {
            mergeFileIds->Append(item.Key);
        }
        else if (deletedEntriesEnabled && IsFileQualifiedForDeletedEntriesMergePolicy(item))
        {
            mergeFileIds->Append(item.Key);
        }
    }

    return mergeFileIds;
}

bool MergeHelper::IsFileQualifiedForInvalidEntriesMergePolicy(__in Data::KeyValuePair<ULONG32, FileMetadata::SPtr> item)
{
    FileMetadata::SPtr fileMetadata = item.Value;

    // TODO: Fix the workaround below until item 5861402 is fixed.
    LONG64 numValidEntries = fileMetadata->NumberOfValidEntries;
    if (numValidEntries < 0)
    {
        numValidEntries = 0;
    }

    LONG64 numInvalidEntries = fileMetadata->TotalNumberOfEntries - numValidEntries;

    if (NumberOfInvalidEntries > 0)
    {
        if (numInvalidEntries >= NumberOfInvalidEntries)
        {
            return true;
        }
    }
    else
    {
        // Calculate percentage of invalid entries
        auto percentageOfInvalidEntries = static_cast<ULONG32>((numInvalidEntries * 100.0f) / fileMetadata->TotalNumberOfEntries);

        if (percentageOfInvalidEntries > percentageOfInvalidEntriesPerFile_)
        {
            return true;
        }
    }

    return false;
}

bool MergeHelper::IsFileQualifiedForDeletedEntriesMergePolicy(__in Data::KeyValuePair<ULONG32, FileMetadata::SPtr> item)
{
    FileMetadata::SPtr fileMetadata = item.Value;
    LONG64 numDeletedEntries = fileMetadata->NumberOfDeletedEntries;

    if (numDeletedEntries > 0)
    {
        auto percentageOfDeletedEntries = static_cast<ULONG32>((numDeletedEntries * 100.0f) / fileMetadata->TotalNumberOfEntries);

        if (percentageOfDeletedEntries > percentageOfDeletedEntriesPerFile_)
        {
            return true;
        }
    }

    return false;
}


ktl::Awaitable<bool> MergeHelper::ShouldMergeDueToFileCountPolicy(
    __in MetadataTable& mergeTable, 
    __out KSharedArray<ULONG32>::SPtr& filesToBeMerged)
{
    auto count = mergeTable.Table->Count;
    auto threshold = fileCountMergeConfigurationSPtr_->FileCountMergeThreshold;
    // If total number of checkpoint files is below the merge threshold, merge is not required.
    if (count < threshold)
    {
        co_return false;
    }

    // Following code assumes that there is one ShouldMerge call at a time.
    AssertIfMapIsNotClean();

    auto enumeratorSPtr = mergeTable.Table->GetEnumerator();
    while(enumeratorSPtr->MoveNext())
    {
        auto currentItem = enumeratorSPtr->Current();
        ULONG32 fileId = currentItem.Key;
        FileMetadata::SPtr fileMetadataSPtr = currentItem.Value;

        ULONG64 fileSize = co_await fileMetadataSPtr->GetFileSizeAsync();

        ULONG32 fileType = fileCountMergeConfigurationSPtr_->GetFileType(fileSize);
        bool listExists = fileTypeToMergeList_->ContainsKey(fileType);
        if (listExists == false)
        {
            KSharedArray<ULONG32>::SPtr list = _new(MERGEHELPER_TAG, GetThisAllocator()) KSharedArray<ULONG32>();
            ASSERT_IFNOT(list != nullptr, "list should not be null");
            fileTypeToMergeList_->Add(fileType, list);
        }

        KSharedArray<ULONG32>::SPtr fileIds = nullptr;
        bool result = fileTypeToMergeList_->TryGetValue(fileType, fileIds);
        ASSERT_IFNOT(result, "Didn't find file type");
        auto status = fileIds->Append(fileId);
        ASSERT_IFNOT(NT_SUCCESS(status), "Unable to append file id to merge list");

        if (fileIds->Count() == fileCountMergeConfigurationSPtr_->FileCountMergeThreshold)
        {
            bool isRemoved = fileTypeToMergeList_->Remove(fileType);
            ASSERT_IFNOT(isRemoved, "Should have been removed");
            CleanMap();
            filesToBeMerged = fileIds;
            co_return true;
        }
    }

    CleanMap();
    co_return false;
}

ktl::Awaitable<bool> MergeHelper::ShouldMergeForSizeOnDiskPolicy(__in MetadataTable& mergeTable)
{
    ULONG numberOfDeltaDifferentialStates = 3;
    if (mergeTable.Table->Count < numberOfDeltaDifferentialStates)
    {
        co_return false;
    }

    ULONG64 totalSize = 0;
    bool hasInvalidOrDeletedEntry = false;

    auto mergeTableEnumerator = mergeTable.Table->GetEnumerator();
    while (mergeTableEnumerator->MoveNext())
    {
        auto fileMetadata = mergeTableEnumerator->Current().Value;
        totalSize += co_await fileMetadata->GetFileSizeAsync();
        hasInvalidOrDeletedEntry |=
            (fileMetadata->NumberOfValidEntries != fileMetadata->TotalNumberOfEntries) ||
            (fileMetadata->NumberOfDeletedEntries > 0);
    }

    co_return (totalSize >= SizeOnDiskThreshold) && hasInvalidOrDeletedEntry;
}

bool MergeHelper::IsMergePolicyEnabled(__in MergePolicy mergePolicy)
{
    ULONG32 tmpMergePolicy = static_cast<ULONG32>(mergePolicy);
    return (static_cast<ULONG32>(CurrentMergePolicy) & tmpMergePolicy) == tmpMergePolicy;
}

void MergeHelper::CleanMap()
{
    auto enumeratorSPtr = fileTypeToMergeList_->GetEnumerator();

    while (enumeratorSPtr->MoveNext())
    {
        auto currentItem = enumeratorSPtr->Current();
        auto list = currentItem.Value;
        list->Clear();
    }
}

void MergeHelper::AssertIfMapIsNotClean()
{
    auto enumeratorSPtr = fileTypeToMergeList_->GetEnumerator();

    while (enumeratorSPtr->MoveNext())
    {
        auto currentItem = enumeratorSPtr->Current();
        auto list = currentItem.Value;
        ASSERT_IFNOT(list->Count() == 0, "List should be empty");
    }
}
