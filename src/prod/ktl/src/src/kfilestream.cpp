//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>

#if defined(K_UseResumable)

using namespace ktl;
using namespace ktl::io;
using namespace ktl::kasync;
using namespace ktl::kservice;

KActivityId PtrToActivityId(VOID* ptr)
{
    return static_cast<KActivityId>(reinterpret_cast<ULONGLONG>(ptr));
}

ULONGLONG SafeCastToULONGLONG(__in LONGLONG l)
{
    KInvariant(l >= 0);
    return static_cast<ULONGLONG>(l);
}

ULONG SafeCastToULONG(__in LONGLONG l)
{
    KInvariant(l >= 0);
    KInvariant(l <= ULONG_MAX);
    return static_cast<ULONG>(l);
}

KFileStream::BufferState::BufferState(__in ULONG BufferSize, __in KAllocator& Allocator)
  : _IsInitialized(FALSE),
    _IsValid(TRUE),
    _IsFlushNeeded(FALSE),
    _FilePositionBase(0),
    _LowestWrittenOffset(0),
    _HighestWrittenOffset(0)
{
    if (BufferSize % KBlockFile::BlockSize != 0)
    {
        SetConstructorStatus(STATUS_INVALID_PARAMETER);
        return;
    }

    KIoBuffer::SPtr ioBuffer;
    KIoBufferView::SPtr writeView;

    NTSTATUS status = KIoBuffer::CreateEmpty(ioBuffer, Allocator);

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    ULONG numBlocks = BufferSize / KBlockFile::BlockSize;

    for(ULONG block = 0; block < numBlocks; block++)
    {
        KIoBufferElement::SPtr element;
        VOID* buffer;

        status = KIoBufferElement::CreateNew(KBlockFile::BlockSize, element, buffer, Allocator);

        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }

        ioBuffer->AddIoBufferElement(*element);
    }

    status = KIoBufferView::Create(writeView, Allocator);

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _IoBuffer = KIoBuffer::SPtr(Ktl::Move(ioBuffer));
    _IoView = KIoBufferView::SPtr(Ktl::Move(writeView));
}

NTSTATUS
KFileStream::BufferState::Create(
    __out BufferState::UPtr& readerWriter,
    __in ULONG BufferSize,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag)
{
    NTSTATUS status;
    BufferState* output;
       
    readerWriter = nullptr;
       
    output = _new(AllocationTag, Allocator) BufferState(BufferSize, Allocator);
       
    if (! output)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }
       
    if (! NT_SUCCESS(output->Status()))
    {
        status = output->Status();
        _delete(output);
        return status;
    }
       
    readerWriter = output;
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::MoveBufferAsync(__in LONGLONG FileOffset)
{
    KInvariant(FileOffset >= 0);
    KInvariant(_BufferState != nullptr);

    NTSTATUS status = STATUS_SUCCESS;
    if (_BufferState->_IsFlushNeeded)
    {
        KInvariant(_BufferState->_IsValid);

        status = co_await FlushBufferAsync();

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "FlushBufferAsync", status);
            co_return status;
        }
    }
    KInvariant(!_BufferState->_IsFlushNeeded); // Flush must be called (if needed) before the buffer is invalidated

    _BufferState->_FilePositionBase = FileOffset - (FileOffset % KBlockFile::BlockSize);

    // If the buffer is being moved to a position >= EOF, no need to read any data.  Reads will still fail from this buffer
    // due to EOF so the user is protected against invalid reads.
    if (_BufferState->_FilePositionBase >= _CachedEOF)
    {
        _BufferState->_HighestWrittenOffset = 0;
        _BufferState->_LowestWrittenOffset = 0;
        _BufferState->_IsValid = TRUE;
        _BufferState->_IsInitialized = TRUE;

        co_return STATUS_SUCCESS;
    }
    
    KInvariant(_BufferState->_FilePositionBase < _CachedEOF);

    LONGLONG eofRoundedUpToBlock;
    if (_CachedEOF % KBlockFile::BlockSize == 0)
    {
        eofRoundedUpToBlock = _CachedEOF;
    }
    else
    {
        eofRoundedUpToBlock = _CachedEOF + (KBlockFile::BlockSize - _CachedEOF % KBlockFile::BlockSize);
    }

    LONGLONG readSize = __min(_BufferState->_IoBuffer->QuerySize(), eofRoundedUpToBlock - _BufferState->_FilePositionBase);
    _BufferState->_IoView->SetView(*_BufferState->_IoBuffer, 0, SafeCastToULONG(readSize));

    KDbgPrintfInformational(
        "Reading %lu bytes at position %lld into buffer, current eof=%lld, eofRoundedUpToBlock=%lld, fileOffset=%lld",
        _BufferState->_IoBuffer->QuerySize(),
        _BufferState->_FilePositionBase,
        _CachedEOF,
        eofRoundedUpToBlock,
        FileOffset);
    
    status = co_await _File->TransferAsync(
        KBlockFile::IoPriority::eForeground,
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        KBlockFile::TransferType::eRead,
        _BufferState->_FilePositionBase,
        *_BufferState->_IoView);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWData(
            PtrToActivityId(this),
            "TransferAsync. CachedEOF: %llu, CachedFileMetadataEOF: %llu, Position: %llu, FilePositionBase: %llu",
            status,
            _CachedEOF,
            _CachedFileMetadataEOF,
            _Position,
            _BufferState->_FilePositionBase);

        LONGLONG actualEOF = -1;
        co_await _File->GetEndOfFileAsync(actualEOF, nullptr, nullptr);
        KDbgErrorWData(PtrToActivityId(this), "", 0, _File->QueryFileSize(), actualEOF, 0, 0);

        co_return status;
    }

    KDbgPrintf("Moved buffer to file position %lld", _BufferState->_FilePositionBase);

    _BufferState->_HighestWrittenOffset = 0;
    _BufferState->_LowestWrittenOffset = 0;
    _BufferState->_IsInitialized = TRUE;
    _BufferState->_IsValid = TRUE;

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::ExtendFileSizeAndEOF(__in LONGLONG potentialEOF)
{
    ULONGLONG fileSize = _File->QueryFileSize();
    KAssert(fileSize <= LONGLONG_MAX);
    if (_CachedFileMetadataEOF < potentialEOF || static_cast<LONGLONG>(_File->QueryFileSize()) < potentialEOF)
    {
        // KBlockFile will handle rounding to a block boundary
        NTSTATUS status = co_await _File->SetFileSizeAsync(potentialEOF + _FileLengthPadSize);
        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "SetFileSizeAsync", status);
            co_return status;
        }

        fileSize = _File->QueryFileSize();
        KAssert(fileSize <= LONGLONG_MAX);
        _CachedFileMetadataEOF = static_cast<LONGLONG>(_File->QueryFileSize()); // get the rounded value
    }

    co_return STATUS_SUCCESS;
}

// assumption: VDL/eof was already increased by the call to WriteAsync
Awaitable<NTSTATUS> KFileStream::FlushBufferAsync()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!_BufferState->_IsValid)
    {
        KDbgCheckpoint(PtrToActivityId(this), "FlushBufferAsync requested for invalidated buffer");
        co_return STATUS_SUCCESS;
    }

    if (!_BufferState->_IsFlushNeeded)
    {
        co_return STATUS_SUCCESS;
    }

    KInvariant(_BufferState->_IsInitialized);  // If a flush was needed, the buffer must have been initialized
    KInvariant(_BufferState->_HighestWrittenOffset >= _BufferState->_LowestWrittenOffset);
    
    // round the written offsets to the nearest block
    ULONG lowestBlockOffset = _BufferState->_LowestWrittenOffset - _BufferState->_LowestWrittenOffset % KBlockFile::BlockSize;
    ULONG highestBlockOffset = _BufferState->_HighestWrittenOffset + (KBlockFile::BlockSize - _BufferState->_HighestWrittenOffset % KBlockFile::BlockSize);
    _BufferState->_IoView->SetView(*_BufferState->_IoBuffer, lowestBlockOffset, highestBlockOffset - lowestBlockOffset);

    status = co_await _File->TransferAsync(
        KBlockFile::IoPriority::eForeground,
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        KBlockFile::eWrite,
        _BufferState->_FilePositionBase + lowestBlockOffset,
        *_BufferState->_IoView);

    if (!NT_SUCCESS(status))
    {
        _IsFaulted = TRUE;
        KDbgErrorWStatus(PtrToActivityId(this), "TransferAsync", status);
        co_return status;
    }

    _BufferState->_IsFlushNeeded = FALSE;

    co_return STATUS_SUCCESS;
}

KFileStream::KFileStream()
    : _Position(0),
      _IsFaulted(FALSE)
{
}

KFileStream::~KFileStream()
{
    _File = nullptr;
    _BufferState = nullptr;
}

KDefineDefaultCreate(KFileStream, FileStream);

Awaitable<NTSTATUS> KFileStream::OpenAsync(
    __in KBlockFile& File,
    __in_opt ULONG BufferSize,
    __in_opt ULONG FileLengthPadSize,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OpenAsync");

    if (_File != nullptr)
    {
        // Open can only be called once.
        co_return STATUS_SHARING_VIOLATION;
    }

    NTSTATUS status;
    
    OpenAwaiter::SPtr awaiter;
    status = OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        ParentAsync,
        GlobalContextOverride);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "OpenAwaiter::Create", status);
        co_return status;
    }
    
    _File = &File;
    _BufferSize = BufferSize;
    _FileLengthPadSize = FileLengthPadSize;

    co_return co_await *awaiter;
}

VOID KFileStream::OnServiceOpen()
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OnServiceOpen");
    SetDeferredCloseBehavior();

    OpenTask();
}

Task KFileStream::OpenTask()
{
    NTSTATUS status;

    HANDLE fileHandle;
    _File->QueryFileHandle(fileHandle);

#if defined(PLATFORM_UNIX)
    KDbgPrintfInformational(
        "Opening KFileStream. Stream ptr: %llx File ptr: %llx File handle: %llx Filename: %s",
        PtrToActivityId(this),
        PtrToActivityId(_File.RawPtr()),
        PtrToActivityId(fileHandle),
        Utf16To8(_File->GetFileName()).c_str());
#else
    KDbgPrintfInformational(
        "Opening KFileStream. Stream ptr: %llx File ptr: %llx File handle: %llx Filename: %S",
        PtrToActivityId(this),
        PtrToActivityId(_File.RawPtr()),
        PtrToActivityId(fileHandle),
        _File->GetFileName());
#endif

    LONGLONG eof;
    status = co_await _File->GetEndOfFileAsync(eof);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "_File->GetEndOfFileAsync", status);
        CompleteOpen(status);
        co_return;
    }

    _CachedEOF = eof;
    _CachedFileMetadataEOF = eof;

    CompleteOpen(STATUS_SUCCESS);
}

Awaitable<NTSTATUS> KFileStream::CloseAsync()
{
    KDbgCheckpointInformational(PtrToActivityId(this), "CloseAsync");

    NTSTATUS status;

    CloseAwaiter::SPtr awaiter;
    status = CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this, 
        awaiter,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "CloseAwaiter::Create", status);
        co_return status;
    }

    co_return co_await *awaiter;
}

VOID KFileStream::OnServiceClose()
{
    KDbgCheckpointInformational(PtrToActivityId(this), "OnServiceClose");
    CloseTask();
}

Task KFileStream::CloseTask()
{
    KDbgCheckpointInformational(PtrToActivityId(this), "CloseTask");

    NTSTATUS status;

    // "explicit" flush, will set EOF on-disk
    status = co_await InternalFlushAsync();
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "InternalFlushAsync", status);
        CompleteClose(status);
        co_return;
    }

    _File = nullptr;
    _BufferState = nullptr;
    _Position = 0;

    CompleteClose(STATUS_SUCCESS);
}

LONGLONG KFileStream::GetPosition() const
{
    KInvariant(_Position >= 0);
    return _Position;
}

VOID KFileStream::SetPosition(__in LONGLONG Position)
{
    KInvariant(Position >= 0);
    KInvariant(_Position >= 0);
    _Position = Position;
}

LONGLONG KFileStream::GetLength() const
{
    return _CachedEOF;
}

NTSTATUS KFileStream::InvalidateForTruncate(__in LONGLONG EndOfFile)
{
    KService$ApiEntry(TRUE);

    if (_IsFaulted)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "InvalidateForTruncate unavailable: stream in faulted state.", status);
        return status;
    }

    if (EndOfFile < 0)
    {
        KDbgErrorWData(
            PtrToActivityId(this),
            "EndOfFile must be nonnegative. EndOfFile: %lld",
            K_STATUS_OUT_OF_BOUNDS,
            EndOfFile,
            0,
            0,
            0);

        return STATUS_INVALID_PARAMETER;
    }

    if (_BufferState != nullptr && _BufferState->_IsFlushNeeded)
    {
        KDbgErrorWData(
            PtrToActivityId(this),
            "Invalidation requested when unflushed writes are present. EndOfFile: %lld  FilePositionBase: %lld  LowestWrittenOffset: %lu  HighestWrittenOffset: %lu",
            STATUS_INVALID_STATE_TRANSITION,
            EndOfFile,
            _BufferState->_FilePositionBase,
            _BufferState->_LowestWrittenOffset,
            _BufferState->_HighestWrittenOffset);

        return STATUS_INVALID_STATE_TRANSITION;
    }

    if (_BufferState != nullptr)
    {
        LONGLONG bufferFilePosition = _BufferState->_FilePositionBase;
        LONGLONG bufferFilePositionEnd = bufferFilePosition + _BufferState->_IoBuffer->QuerySize();

        if ((EndOfFile >= bufferFilePosition && EndOfFile < bufferFilePositionEnd) // EndOfFile intersects with buffer
            || (EndOfFile < bufferFilePosition)) // EndOfFile is less than buffer
        {
            _BufferState->_IsValid = FALSE;
        }
    }

    _CachedEOF = EndOfFile;
    _CachedFileMetadataEOF = EndOfFile;

    return STATUS_SUCCESS;
}

NTSTATUS KFileStream::InvalidateForWrite(__in LONGLONG FilePosition, __in LONGLONG Count)
{
    KService$ApiEntry(TRUE);

    if (_IsFaulted)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "InvalidateForWrite unavailable: stream in faulted state.", status);
        return status;
    }

    if (FilePosition < 0)
    {
        KDbgErrorWData(
            PtrToActivityId(this),
            "FilePosition must be nonnegative. FilePosition: %lld",
            K_STATUS_OUT_OF_BOUNDS,
            FilePosition,
            0,
            0,
            0);

        return STATUS_INVALID_PARAMETER_1;
    }

    if (Count <= 0)
    {
        KDbgErrorWData(
            PtrToActivityId(this), 
            "Count must be nonnegative and nonzero. Count: %lld",
            K_STATUS_OUT_OF_BOUNDS,
            Count,
            0,
            0,
            0);

        return STATUS_INVALID_PARAMETER_2;
    }

    if (_BufferState != nullptr && _BufferState->_IsFlushNeeded)
    {
        KDbgErrorWData(
            PtrToActivityId(this),
            "Invalidation requested when unflushed writes are present. FilePosition: %lld  FilePositionBase: %lld  LowestWrittenOffset: %lu  HighestWrittenOffset: %lu",
            STATUS_INVALID_STATE_TRANSITION,
            FilePosition,
            _BufferState->_FilePositionBase,
            _BufferState->_LowestWrittenOffset,
            _BufferState->_HighestWrittenOffset);

        return STATUS_INVALID_STATE_TRANSITION;
    }

    // assumption: Invalidate is only called after a successful EXPLICITLY flushed 
    // (via Flush or Close) write to the file, which (if it went past EOF) would have updated the file's metadata with the exact (correct) EOF
    LONGLONG potentialNewEof = FilePosition + Count;
    if (potentialNewEof > _CachedEOF)
    {
        _CachedEOF = potentialNewEof;
    }
    if (potentialNewEof > _CachedFileMetadataEOF)
    {
        _CachedFileMetadataEOF = potentialNewEof;
    }

    if (_BufferState != nullptr)
    {
        LONGLONG filePositionEnd = FilePosition + Count;
        LONGLONG bufferFilePosition = _BufferState->_FilePositionBase;
        LONGLONG bufferFilePositionEnd = bufferFilePosition + _BufferState->_IoBuffer->QuerySize();

        if ((FilePosition >= bufferFilePosition && FilePosition < bufferFilePositionEnd) // FilePosition intersects with buffer
            || (filePositionEnd >= bufferFilePosition && filePositionEnd < bufferFilePositionEnd) // filePositionEnd intersects with buffer
            || (FilePosition <= bufferFilePosition && filePositionEnd >= bufferFilePositionEnd)) // write spans buffer
        {
            _BufferState->_IsValid = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::ReadAsync(
    __in KBuffer& Buffer,
    __out ULONG& BytesRead,
    __in ULONG OffsetIntoBuffer,
    __in ULONG Count)
{
    KCoService$ApiEntry(TRUE);

    KDbgPrintfInformational("ReadAsync offset=%lu count=%lu position=%lld", OffsetIntoBuffer, Count, _Position);

    NTSTATUS status;

    BytesRead = 0;

    if (_IsFaulted)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "ReadAsync unavailable: stream in faulted state.", status);
        co_return status;
    }

    if (_BufferState == nullptr)
    {
        status = BufferState::Create(_BufferState, _BufferSize, GetThisAllocator(), GetThisAllocationTag());
        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "BufferState::Create", status);
            co_return status;
        }
    }

    if (OffsetIntoBuffer >= Buffer.QuerySize())
    {
        co_return STATUS_INVALID_PARAMETER_2;
    }

    // if count > the rest of the space in the buffer, fill the rest of the buffer
    Count = __min(Count, Buffer.QuerySize() - OffsetIntoBuffer);

    if (Count == 0)
    {
        Count = Buffer.QuerySize() - OffsetIntoBuffer;
    }

    if (_Position >= _CachedEOF)
    {
        KDbgPrintf(
            "Read requested >= EOF.  Reading 0 bytes.  File position: %lld, EOF: %lld",
            _Position,
            _CachedEOF);

        co_return STATUS_SUCCESS;
    }

    // if read would extend past EOF, only read until EOF
    if (_CachedEOF < _Position + Count)
    {
        ULONG oldCount = Count;
        KInvariant(_Position < _CachedEOF);
        Count = SafeCastToULONG(_CachedEOF - _Position);
        KDbgPrintf(
            "Reading up to EOF since the full read could not be served.  EOF: %lld, Old Count: %lu, New Count: %lu",
            _CachedEOF,
            oldCount,
            Count);
    }

    // If the buffer has been invalidated or has not been initialized, read from disk
    if (!_BufferState->_IsValid || !_BufferState->_IsInitialized)
    {
        KInvariant(!_BufferState->_IsFlushNeeded);

        status = co_await MoveBufferAsync(_Position);

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
            co_return status;
        }
    }

    // Flush if necessary to prevent reading unflushed writes
    if (_BufferState->_IsFlushNeeded)
    {
        KInvariant(_BufferState->_IsValid);

        status = co_await FlushBufferAsync();

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "FlushBufferAsync", status);
            co_return status;
        }
    }

    if (_Position < _BufferState->_FilePositionBase)
    {
        // Out of range (read a position less than the lowest position)

        status = co_await MoveBufferAsync(_Position);

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
            co_return status;
        }
    }

    LONGLONG potentialBufferOffset;
    ULONG actualBufferOffset;
    ULONG bytesRead = 0;
    PUCHAR outPtr = static_cast<PUCHAR>(Buffer.GetBuffer()) + OffsetIntoBuffer;

    while (bytesRead < Count)
    {
        KAssert((_Position + bytesRead) >= 0);
        KAssert((_Position + bytesRead) >= _BufferState->_FilePositionBase);

        potentialBufferOffset = (_Position + bytesRead) - _BufferState->_FilePositionBase;

        KAssert(potentialBufferOffset >= 0);

        if (potentialBufferOffset >= _BufferState->_IoBuffer->QuerySize())
        {
            // Out of range (next read position is beyond the end of the current buffer)

            status = co_await MoveBufferAsync(_BufferState->_FilePositionBase + potentialBufferOffset);

            if (!NT_SUCCESS(status))
            {
                KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
                co_return status;
            }

            KInvariant((_Position + bytesRead) >= _BufferState->_FilePositionBase);
            actualBufferOffset = SafeCastToULONG((_Position + bytesRead) - _BufferState->_FilePositionBase);
        }
        else
        {
            actualBufferOffset = SafeCastToULONG(potentialBufferOffset);
        }
        KInvariant(actualBufferOffset < _BufferState->_IoBuffer->QuerySize());

        // File size was checked above, so we won't be reading invalid data (Count - bytesRead will hit 0 before a filePosition >= EOF will be reached in the buffer)
        ULONG toRead = __min(_BufferState->_IoBuffer->QuerySize() - actualBufferOffset, Count - bytesRead);
        KInvariant(toRead > 0);

        BOOL res = KIoBufferStream::CopyFrom(*_BufferState->_IoBuffer, actualBufferOffset, toRead, outPtr + bytesRead);

        if (!res)
        {
            KDbgErrorWStatus(PtrToActivityId(this), "KIoBufferStream::CopyFrom", STATUS_UNSUCCESSFUL);
            co_return STATUS_UNSUCCESSFUL;
        }

        bytesRead += toRead;
    }

    BytesRead = bytesRead;
    _Position += BytesRead;

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::ReadAsync(__out KIoBuffer& Buffer)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;

    if (_IsFaulted)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "ReadAsync unavailable: stream in faulted state.", status);
        co_return status;
    }

    if (_Position >= _CachedEOF)
    {
        status = STATUS_END_OF_FILE;
        KDbgErrorWData(
            PtrToActivityId(this),
            "Block-aligned read requested beyond EOF.  Position: %lld  EOF: %lld",
            status,
            _Position,
            _CachedEOF,
            0,
            0);

        co_return status;
    }

    // flush if necessary
    if (_BufferState != nullptr)
    {
        status = co_await FlushBufferAsync();

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "FlushBufferAsync", status);
            co_return status;
        }
    }

    status = co_await _File->TransferAsync(
        KBlockFile::IoPriority::eForeground,
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        KBlockFile::TransferType::eRead,
        _Position,
        Buffer);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "File->TransferAsync", status);
        co_return status;
    }

    _Position += Buffer.QuerySize();

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::WriteAsync(
    __in KBuffer const & Buffer,
    __in ULONG OffsetIntoBuffer,
    __in ULONG Count)
{
    KCoService$ApiEntry(TRUE);

    KDbgPrintfInformational("WriteAsync offset=%lu count=%lu position=%lld", OffsetIntoBuffer, Count, _Position);

    NTSTATUS status;

    if (_IsFaulted)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "WriteAsync unavailable: stream in faulted state.", status);
        co_return status;
    }

    if (_BufferState == nullptr)
    {
        status = BufferState::Create(_BufferState, _BufferSize, GetThisAllocator(), GetThisAllocationTag());
        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "BufferState::Create", status);
            co_return status;
        }
    }

    if (Buffer.QuerySize() < OffsetIntoBuffer + Count)
    {
        KDbgPrintfInformational(
            "Write length is larger than buffer span provided.  Write length: %lu, OffsetIntoBuffer: %lu, Buffer length: %lu, Span length: %lu",
            Count,
            OffsetIntoBuffer,
            Buffer.QuerySize(),
            Buffer.QuerySize() - OffsetIntoBuffer);

        co_return STATUS_BUFFER_TOO_SMALL;
    }

    if (Count == 0)
    {
        Count = Buffer.QuerySize() - OffsetIntoBuffer;
        KDbgPrintfInformational("Count provided == 0, the rest of the buffer will be considered as the length.  Read length: %lu", Count);
    }

    KInvariant(Count > 0);

    if (_Position < _BufferState->_FilePositionBase)
    {
        // Out of range (read a position less than the lowest position)

        status = co_await MoveBufferAsync(_Position);

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
            co_return status;
        }
    }

    // If the buffer has been invalidated, re-read from disk before writing
    if (!_BufferState->_IsValid || !_BufferState->_IsInitialized)
    {
        KInvariant(!_BufferState->_IsFlushNeeded);

        status = co_await MoveBufferAsync(_Position);

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
            co_return status;
        }
    }

    // Extend filesize/eof if necessary
    LONGLONG potentialEOF = _Position + Count;
    status = co_await ExtendFileSizeAndEOF(potentialEOF);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "ExtendFileSizeAndEOF", status);
        co_return status;
    }

    LONGLONG oldEOF = _CachedEOF;
    _CachedEOF = __max(_CachedEOF, _Position + Count); // Optimistically update the cached EOF since the new value is needed in MoveBufferAsync.  If there is a failure, set back to the previous value

    ULONG myBufferOffset;
    ULONG bytesWritten = 0;
    PCUCHAR inPtr = static_cast<PCUCHAR>(Buffer.GetBuffer()) + OffsetIntoBuffer;

    while (bytesWritten < Count)
    {
        myBufferOffset = SafeCastToULONG((_Position + bytesWritten) - _BufferState->_FilePositionBase);
        if (myBufferOffset >= _BufferState->_IoBuffer->QuerySize())
        {
            // Out of range (next write position is beyond the end of the current buffer)

            status = co_await MoveBufferAsync(_BufferState->_FilePositionBase + myBufferOffset); // will flush if necessary

            if (!NT_SUCCESS(status))
            {
                KDbgErrorWStatus(PtrToActivityId(this), "MoveBufferAsync", status);
                _CachedEOF = oldEOF;
                co_return status;
            }

            KInvariant(_BufferState->_FilePositionBase <= _Position + bytesWritten);
            myBufferOffset = SafeCastToULONG((_Position + bytesWritten) - _BufferState->_FilePositionBase);
        }
        
        KInvariant(myBufferOffset < _BufferState->_IoBuffer->QuerySize());

        ULONG toWrite = __min(_BufferState->_IoBuffer->QuerySize() - myBufferOffset, Count - bytesWritten);
        KInvariant(toWrite > 0);

        BOOL res = KIoBufferStream::CopyTo(*_BufferState->_IoBuffer, myBufferOffset, toWrite, inPtr + bytesWritten);

        if (!res)
        {
            status = K_STATUS_OUT_OF_BOUNDS;
            KDbgErrorWStatus(PtrToActivityId(this), "KIoBufferStream::CopyTo", status);
            _CachedEOF = oldEOF;
            co_return STATUS_UNSUCCESSFUL;
        }

        if (myBufferOffset < _BufferState->_LowestWrittenOffset)
        {
            _BufferState->_LowestWrittenOffset = myBufferOffset;
        }

        if (myBufferOffset + toWrite - 1 > _BufferState->_HighestWrittenOffset)
        {
            _BufferState->_HighestWrittenOffset = myBufferOffset + toWrite - 1;
        }

        bytesWritten += toWrite;
        _BufferState->_IsFlushNeeded = TRUE;
    }

    _Position += Count;

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::WriteAsync(__in KIoBuffer& Buffer)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status = STATUS_SUCCESS;

    if (_IsFaulted)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "WriteAsync unavailable: stream in faulted state.", status);
        co_return status;
    }

    // flush if necessary
    if (_BufferState != nullptr)
    {
        status = co_await FlushBufferAsync();

        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "FlushBufferAsync", status);
            co_return status;
        }
    }

    // Extend filesize/eof if necessary
    LONGLONG potentialEOF = _Position + Buffer.QuerySize();
    status = co_await ExtendFileSizeAndEOF(potentialEOF);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "ExtendFileSizeAndEOF", status);
        co_return status;
    }

    status =  co_await _File->TransferAsync(
        KBlockFile::IoPriority::eForeground,
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        KBlockFile::eWrite,
        _Position,
        Buffer);

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "File->TransferAsync", status);
        co_return status;
    }

    // invalidate if necessary since this write did not touch the buffer.  Implicitly updates the cached EOF if necessary
    status = InvalidateForWrite(_Position, Buffer.QuerySize());

    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "InvalidateForWrite", status);
        co_return status;
    }

    _Position += Buffer.QuerySize();
    KAssert(_CachedEOF == __max(_CachedEOF, _Position)); // _CachedEOF was updated, if necessary, by InvalidateForWrite

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> KFileStream::FlushAsync()
{
    KCoService$ApiEntry(TRUE);

    KDbgPrintfInformational("FlushAsync");

    NTSTATUS status;

    if (_IsFaulted)
    {
        status = STATUS_UNSUCCESSFUL;
        KDbgErrorWStatus(PtrToActivityId(this), "FlushAsync unavailable: stream in faulted state.", status);
        co_return status;
    }

    co_return co_await InternalFlushAsync();
}

Awaitable<NTSTATUS> KFileStream::InternalFlushAsync()
{
    NTSTATUS status;

    if (_BufferState != nullptr)
    {
        status = co_await FlushBufferAsync();

        if (!NT_SUCCESS(status))
        {
            KAssert(_IsFaulted == TRUE);
            KDbgErrorWStatus(PtrToActivityId(this), "FlushBufferAsync", status);
            co_return status;
        }
    }

    // If the file was opened with writethrough disabled, it needs to be flushed here to ensure all writes are flushed to disk.
    // NOTE: To meet any guarantees about what is on disk at any particular time, we only support devices (+ virtualized devices)
    // which support correctly implemented FUA and flush, and OS configurations which write-through using FUA.
    if (!_File->IsWriteThrough())
    {
        status = co_await _File->FlushAsync();
        if (!NT_SUCCESS(status))
        {
            _IsFaulted = TRUE;
            KDbgErrorWStatus(PtrToActivityId(this), "KBlockFile::FlushAsync", status);
            co_return status;
        }
    }

    if (_CachedFileMetadataEOF != _CachedEOF)
    {
        status = co_await _File->SetEndOfFileAsync(_CachedEOF, nullptr, nullptr);

        if (!NT_SUCCESS(status))
        {
            _IsFaulted = TRUE;
            KDbgErrorWStatus(PtrToActivityId(this), "SetEndOfFileAsync", status);
            co_return status;
        }

        _CachedFileMetadataEOF = _CachedEOF;
    }

    co_return STATUS_SUCCESS; // If no unaligned writes have been performed, flush is a no-op
}

#endif
