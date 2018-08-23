//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once
#if defined(K_UseResumable)

namespace ktl
{
    namespace io
    {
        //
        //  File-based implementation of KStream.
        //
        //  IO failures and/or process crash while buffered writes are unflushed via FlushAsync or CloseAsync will leave the file
        //  metadata in an inconsistent state.
        //
        class KFileStream : public KAsyncServiceBase, public KStream
        {
            K_FORCE_SHARED(KFileStream);
            K_SHARED_INTERFACE_IMP(KStream);

        private:

            //
            //  Lazily-allocated buffer + associated state for doing unaligned IO
            //
            struct BufferState : public KObject<BufferState>
            {
                typedef KUniquePtr<BufferState> UPtr;

                FAILABLE
                BufferState(__in ULONG BufferSize, __in KAllocator& Allocator);
                
                static NTSTATUS Create(
                    __out BufferState::UPtr& bufferState,
                    __in ULONG BufferSize,
                    __in KAllocator& Allocator,
                    __in ULONG AllocationTag);

                BOOL _IsInitialized; // False until buffer has been filled with data from disk for the first time.
                BOOL _IsValid; // True iff InvalidateForWrite has been called with a write intersecting the current buffer.  Set to FALSE once the buffer is re-read from disk.
                BOOL _IsFlushNeeded; // True iff unflushed writes are present in _IoBuffer.  Set to FALSE by a successful flush.
                KIoBuffer::SPtr _IoBuffer; // Buffer used to accumulate unaligned writes prior to flush, and to read from the file
                KIoBufferView::SPtr _IoView; // View used to only write a portion of the buffer to the file (only the modified span)
                LONGLONG _FilePositionBase; // The position in the file that buffer offset 0 corresponds to.  Will always be block-aligned and positive.
                ULONG _LowestWrittenOffset; // The lowest offset of user data which has been written to the buffer since the last flush.
                ULONG _HighestWrittenOffset; // The highest offset of user data which has been written to the buffer since the last flush.
            };

        public:

            //
            //  The default size of buffer to allocate for unaligned IO
            //
            static const ULONG DefaultBufferSize = KBlockFile::BlockSize * 32;

            //
            //  The default number of bytes to pad the filesize by when extending the file size to accomodate a write
            //
            static const ULONG DefaultFileLengthPadSize = KBlockFile::BlockSize * 32;

            //
            //  Create a KFileStream instance
            //
            KDeclareDefaultCreate(KFileStream, FileStream, KTL_TAG_FILE);

            //
            //  Open the file stream.
            //
            Awaitable<NTSTATUS> OpenAsync(
                __in KBlockFile& File,
                __in_opt ULONG BufferSize = DefaultBufferSize,
                __in_opt ULONG FileLengthPadSize = DefaultFileLengthPadSize,
                __in_opt KAsyncContextBase* const ParentAsync = nullptr,
                __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr);

            //
            //  Close and flush the file stream.
            //
            Awaitable<NTSTATUS> CloseAsync() override;

            //
            //  Read "Count" bytes from the stream into "Buffer" starting at buffer offset "OffsetIntoBuffer"
            //
            //  If the entire read can be serviced from the current buffer and InvalidateForWrite has not invalidated
            //  the buffer, the read will be serviced without issuing IO.  Otherwise, one or more reads to the disk will
            //  be issued to fill the internal buffer and copy into Buffer until Count bytes have been read.
            //
            //  If a flush is pending (due to an unflushed write) an implicit flush will happen before completing the read, to prevent
            //  reading unflushed writes.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            //  Parameters:
            //      Buffer - The buffer to read into.
            //      BytesRead - The total number of bytes read into Buffer.  If a read is requested at or beyond the end of file, BytesRead will be zero.
            //      OffsetIntoBuffer - The offset into Buffer to read into.
            //      Count - The number of bytes to read.  If 0, a length of Buffer.QuerySize() - OffsetIntoBuffer will be used.
            //      
            Awaitable<NTSTATUS> ReadAsync(
                __out KBuffer& Buffer,
                __out ULONG& BytesRead,
                __in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0) override;

            //
            //  Read __min(EOF - Buffer.QuerySize(), Buffer.QuerySize()) bytes from the stream into "Buffer"
            //
            //  If a flush is pending, an implicit flush will happen before completing the read, to prevent reading unflushed writes.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            //  REMARK: If the user wishes to only read a subset of their KIoBuffer, consider creating/using a KIoBufferView or
            //  reference.
            //
            //  Parameters:
            //      Buffer - The buffer to read into.
            //
            Awaitable<NTSTATUS> ReadAsync(__out KIoBuffer& Buffer);
            
            //
            //  Write "Count" bytes to the stream from "Buffer" starting at buffer offset "OffsetIntoBuffer"
            //
            //  The internal buffer will only be flushed as needed; if the entire write will fit inside the current buffer,
            //  no IO will be issued until Flush or Close is called or another IO is issued that requires a flush (e.g. any Read,
            //  or a write which does not fit within the current buffer).
            //
            //  NOTE: Any writes occuring prior to an explicit flush (via FlushAsync() or CloseAsync()) may leave the EOF in an
            //  inconsistent state.  If a failure occurs and the EOF is not able to be set correctly during Flush/Close, the disk
            //  metadata EOF cannot be relied upon.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            //  Parameters:
            //      Buffer - The buffer to write from.
            //      OffsetIntoBuffer - The offset into Buffer to write from.
            //      Count - The number of bytes to write.  If 0, a length of Buffer.QuerySize() - OffsetIntoBuffer will be used.
            // 
            Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & Buffer, 
                __in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0) override;

            //
            //  Write the contents of "Buffer" to the stream.
            //
            //  NOTE: Any writes occuring prior to an explicit flush (via FlushAsync() or CloseAsync()) may leave the EOF in an
            //  inconsistent state.  If a failure occurs and the EOF is not able to be set correctly during Flush/Close, the disk
            //  metadata EOF cannot be relied upon.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            //  REMARK: If the user wishes to only write a subset of their KIoBuffer, consider creating/using a KIoBufferView or
            //  reference.
            //
            //  Parameters:
            //      Buffer - The buffer to write from.
            Awaitable<NTSTATUS> WriteAsync(__in KIoBuffer & Buffer);

            //
            //  If necessary, flush any buffered writes to disk, and update the disk metadata to reflect the cached EOF.
            //
            //  NOTE: Any writes occuring prior to an explicit flush (via FlushAsync() or CloseAsync()) may leave the EOF in an
            //  inconsistent state.  If a failure occurs and the EOF is not able to be set correctly during Flush/Close, the disk
            //  metadata EOF cannot be relied upon.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            Awaitable<NTSTATUS> FlushAsync() override;

            //
            //  Inform the stream that the underlying file has been truncated to "EndOfFile."  
            //  - If this API is not called after truncation and before calling other APIs, undefined behavior will occur.
            //  - If this API is called with "EndOfFile" being any other value than the current on-disk EOF, undefined behavior will occur.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            NTSTATUS InvalidateForTruncate(__in LONGLONG EndOfFile);

            //
            //  Invalidate the buffer for reads if a write at "FilePosition" with length "Count" would intersect.
            //  - Will fail (by returning a failure status) if there are any unflushed (buffered) writes.
            //  - Will update the cached EOF if this write extends past the current cached EOF.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            NTSTATUS InvalidateForWrite(__in LONGLONG FilePosition, __in LONGLONG Count);

            //
            //  Get the current file offset that the stream will read from and/or write to.
            //
            LONGLONG GetPosition() const override;

            //
            //  Set the file offset that the stream will read from and/or write to.
            //
            //  NOTE: As KFileStream is not threadsafe, only one operation (Read, Write, Flush, SetPosition, Invalidate) may be active at
            //  a time.  Calling any of these APIs concurrently will yield undefined behavior.
            //
            void SetPosition(__in LONGLONG Position) override;

            //
            //  Get the cached EOF.  Will not reflect modifications to the file not performed by this stream unless InvalidateForWrite
            //  was called.
            //
            LONGLONG GetLength() const override;

        protected:

            VOID OnServiceOpen() override;
            VOID OnServiceClose() override;

        private:

            //
            // Perform async work as a part of Open.
            // 
            Task OpenTask();
            
            //
            // Perform async work as a part of Close.
            //
            Task CloseTask();

            //
            // 'Explicit' (user-initiated) flush, will set EOF.  Public APIs which use an explicit flush may call this method.  Does not
            // acquire an activity, so it is suitable for usage during CloseAsync.
            //
            Awaitable<NTSTATUS> InternalFlushAsync();

            //
            // 'Implicit' (non-user-initiated) flush, will not modify EOF.  Does not acquire an activity.
            //
            Awaitable<NTSTATUS> FlushBufferAsync();

            //
            // Move the buffer window to start with the block containing fileOffset
            //
            Awaitable<NTSTATUS> MoveBufferAsync(LONGLONG fileOffset);  

            //
            // Extend filesize + EOF (with padding) if potentialEOF > _CachedFileMetadataEOF or potentialEOF > filesize
            //
            Awaitable<NTSTATUS> ExtendFileSizeAndEOF(LONGLONG potentialEOF); 

            BOOLEAN _IsFaulted; // If the filestream is in a faulted state (e.g. a flush failed), this will be set to TRUE and will guard against future API calls.
            LONGLONG _CachedEOF; // The known end of file, including unflushed writes.  May be increased by calls to WriteAsync and/or InvalidateForWrite.
            LONGLONG _CachedFileMetadataEOF; // The last known value written to the file metadata via SetEndOfFile.  May be less than CachedEOF if there are any unflushed writes
            LONGLONG _Position; // Stream position into the file.  always positive
            KBlockFile::SPtr _File;  // The underlying file
            ULONG _BufferSize; // Size of unaligned IO buffer to allocate
            KUniquePtr<BufferState> _BufferState;  // lazily-initialized state for handling unaligned writes
            ULONG _FileLengthPadSize;  // When increasing the file length on a write, pad the extension by this amount to avoid frequent length increases
        };
    }
}

#endif

