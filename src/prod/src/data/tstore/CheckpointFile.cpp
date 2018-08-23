// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;
using namespace Data::Utilities;

// Reserved for private static CheckpointFile.OpenAsync() or CheckpointFile.CreateAsync().
CheckpointFile::CheckpointFile(
    __in KStringView const& filename,
    __in KeyCheckpointFile& keyCheckpointFile,
    __in ValueCheckpointFile& valueCheckpointFile,
    __in StoreTraceComponent & traceComponent)
    :keyCheckpointFileSPtr_(&keyCheckpointFile),
    valueCheckpointFileSPtr_(&valueCheckpointFile),
    fileNameBaseSPtr_(nullptr),
    keyFileNameSPtr_(nullptr),
    valueFileNameSPtr_(nullptr),
    isFileSizeCached_(false),
    traceComponent_(&traceComponent)
{
    NTSTATUS status = KString::Create(fileNameBaseSPtr_, GetThisAllocator(), filename);
    Diagnostics::Validate(status);

    status = KString::Create(keyFileNameSPtr_, GetThisAllocator(), filename);
    Diagnostics::Validate(status);
    keyFileNameSPtr_->Concat(KeyCheckpointFile::GetFileExtension());

    status = KString::Create(valueFileNameSPtr_, GetThisAllocator(), filename);
    Diagnostics::Validate(status);
    valueFileNameSPtr_->Concat(ValueCheckpointFile::GetFileExtension());
}

CheckpointFile::~CheckpointFile()
{
}

NTSTATUS CheckpointFile::Create(
    __in KStringView const& filename,
    __in KeyCheckpointFile& keyCheckpointFile,
    __in ValueCheckpointFile& valueCheckpointFile,
    __in StoreTraceComponent & traceComponent,
    __in KAllocator& allocator,
    __out CheckpointFile::SPtr& result)
{
    NTSTATUS status;

    SPtr output = _new(CHECKPOINTFILE_TAG, allocator)CheckpointFile(filename, keyCheckpointFile, valueCheckpointFile, traceComponent);

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

ktl::Awaitable<ULONG64> CheckpointFile::GetTotalFileSizeAsync(__in KAllocator& allocator)
{
    if (!isFileSizeCached_)
    {
        ULONG64 keyFileSize = co_await GetFileSizeAsync(*keyFileNameSPtr_, allocator);
        ULONG64 valueFileSize = co_await GetFileSizeAsync(*valueFileNameSPtr_, allocator);
        cachedFileSize_ = keyFileSize + valueFileSize;
        isFileSizeCached_ = true;
    }

    co_return cachedFileSize_;
}

ktl::Awaitable<KSharedPtr<CheckpointFile>> CheckpointFile::OpenAsync(
    __in KStringView& filename,
    __in StoreTraceComponent & traceComponent,
    __in KAllocator& allocator,
    __in bool isValueReferenceType)
{
    KSharedPtr<KeyCheckpointFile> keyFileSPtr = nullptr;
    KSharedPtr<ValueCheckpointFile> valueFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    try
    {
        KSharedPtr<KString> keyFileNameSPtr = nullptr;
        NTSTATUS status = KString::Create(keyFileNameSPtr, allocator, filename);
        Diagnostics::Validate(status);
        keyFileNameSPtr->Concat(KeyCheckpointFile::GetFileExtension());

        KSharedPtr<KString> valueFileNameSPtr = nullptr;
        status = KString::Create(valueFileNameSPtr, allocator, filename);
        Diagnostics::Validate(status);
        valueFileNameSPtr->Concat(ValueCheckpointFile::GetFileExtension());

        keyFileSPtr = co_await KeyCheckpointFile::OpenAsync(allocator, *keyFileNameSPtr, traceComponent, isValueReferenceType);
        valueFileSPtr = co_await ValueCheckpointFile::OpenAsync(allocator, *valueFileNameSPtr, traceComponent);

        CheckpointFile::SPtr checkpointfileSPtr = nullptr;
        status = CheckpointFile::Create(filename, *keyFileSPtr, *valueFileSPtr, traceComponent, allocator, checkpointfileSPtr);
        Diagnostics::Validate(status);
        co_return checkpointfileSPtr;
    }
    catch (ktl::Exception const& e)
    {
        // Ensure the key and value files get disposed quickly if we get an exception.
        // Normally the CheckpointFile would dispose them, but it may not get constructed.
        exception = SharedException::Create(e, allocator);
    }

    if (keyFileSPtr != nullptr)
    {
        co_await keyFileSPtr->CloseAsync();
    }

    if (valueFileSPtr != nullptr)
    {
        co_await valueFileSPtr->CloseAsync();
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}

ktl::Awaitable<ULONG64> CheckpointFile::GetFileSizeAsync(
    __in KString& filename,
    __in KAllocator& allocator)
{
    KBlockFile::SPtr fileSPtr = nullptr;
    ktl::io::KFileStream::SPtr filestreamSPtr = nullptr;
    bool exists = Common::File::Exists(filename.operator LPCWSTR());
    STORE_ASSERT(exists, "Expected Checkpoint file {1} to exist", ToStringLiteral(filename));

    KWString filePath(GetThisAllocator(), filename);

    NTSTATUS status;

    KBlockFile::CreateOptions createOptions = static_cast<KBlockFile::CreateOptions>(KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite | KBlockFile::CreateOptions::eInheritFileSecurity);

    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        FALSE,
        KBlockFile::eOpenExisting,
        createOptions,
        fileSPtr,
        nullptr,
        allocator,
        CHECKPOINTFILE_TAG);
    STORE_ASSERT(NT_SUCCESS(status), "Failed to open file {1}; Status: {2}", ToStringLiteral(filename), status);

    KFinally([&] {fileSPtr->Close();});

    status = ktl::io::KFileStream::Create(filestreamSPtr, allocator, CHECKPOINTFILE_TAG);
    Diagnostics::Validate(status);

    SharedException::CSPtr exception = nullptr;

    LONGLONG size = -1;
    try
    {
        status = co_await filestreamSPtr->OpenAsync(*fileSPtr);
        STORE_ASSERT(NT_SUCCESS(status), "Failed to open filestream; Status: {1}", status);

        size = filestreamSPtr->GetLength();
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, GetThisAllocator());
    }

    if (filestreamSPtr != nullptr && filestreamSPtr->IsOpen())
    {
        status = co_await filestreamSPtr->CloseAsync();
        STORE_ASSERT(NT_SUCCESS(status), "Failed to close filestream; Status: {1}", status);
    }

    if (exception != nullptr)
    {
        throw exception;
    }

    co_return size;
}
