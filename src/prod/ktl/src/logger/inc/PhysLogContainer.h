// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

public:
    virtual VOID
    QueryLogType(__out KWString& LogType) = 0;

    virtual VOID
    QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId) = 0;

    virtual VOID
    QuerySpaceInformation(
        __out_opt ULONGLONG* const TotalSpace,
        __out_opt ULONGLONG* const FreeSpace = nullptr) = 0;

    virtual ULONGLONG QueryReservedSpace() = 0;
    
    virtual ULONGLONG QueryCurrentReservation() = 0;

    virtual ULONG QueryMaxAllowedStreams() = 0;

    virtual ULONG QueryMaxRecordSize() = 0;

    virtual ULONG QueryMaxUserRecordSize() = 0;

    virtual ULONG QueryUserRecordSystemMetadataOverhead() = 0;

    virtual VOID
    QueryLsnRangeInformation(
        __out LONGLONG& LowestLsn,
        __out LONGLONG& HighestLsn,
        __out RvdLogStreamId& LowestLsnStreamId
        ) = 0 ;
    
    virtual VOID
    QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage) = 0;

    virtual VOID
    SetCacheSize(__in LONGLONG CacheSizeLimit) = 0;

    class AsyncCreateLogStreamContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogStreamContext);
    public:

        //
        // This method asynchronously create a new log stream.
        //
        // Parameters:
        //
        //  LogStreamId - Supplies the log stream KGuid id.
        //
        //  LogStreamType - Supplies the log stream type.
        //
        //  LogStream - Returns the in-memory representation of the log stream.
        //
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  CallbackPtr - Supplies an optional callback delegate to be notified
        //      when the log stream is created.
        //
        // Completion status:
        //
        //  STATUS_SUCCESS - The operation completed successfully.
        //
        //  STATUS_LOG_FULL - There is not enough space to record the new log stream information.
        //      The log is still in an usable state.
        //
        //  STATUS_OBJECT_NAME_COLLISION - A log stream with the same stream ID
        //      in the same log already exists.
        //
        //  STATUS_DELETE_PENDING - Subject Stream is in the process of being deleted
        //
        virtual VOID
        StartCreateLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __in const RvdLogStreamType& LogStreamType,
            __out RvdLogStream::SPtr& LogStream,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) = 0;

    class AsyncDeleteLogStreamContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogStreamContext);
    public:

        //
        // This method asynchronously deletes an existing log stream.
        //
        // Parameters:
        //
        //  LogStreamId - Supplies the log stream KGuid id.
        //
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  CallbackPtr - Supplies an optional callback delegate to be notified
        //      when the log stream is deleted.
        //
        // Completion status:
        //
        //  STATUS_SUCCESS - The operation completed successfully.
        //
        //  STATUS_LOG_FULL - There is not enough space to record the log stream deletion information.
        //      The log is still in an usable state.
        //
        //  STATUS_NOT_FOUND - The speficied log stream does not exist.
        //
        //  NOTES: This operation is potentially long running in that it will not complete (successfully)
        //         until any interest in the underlying RvdLogStream instance has been released - the point
        //         at which time that such RvdLogStream instance would normally close - meaning all refs, both
        //         direct (RvdLogStream::SPtrs) and indirect (RvdLogStream::Async*Contexts), have been released.
        //         Only then will the such an outsatnding StartDeleteLogStream operation complete.
        //
        virtual VOID
        StartDeleteLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) = 0;

    class AsyncOpenLogStreamContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogStreamContext);
    public:
        //
        //
        // This method opens an existing log stream.
        //
        // Parameters:
        //
        //  LogStreamId - Supplies the log stream KGuid id.
        //
        //  LogStream - Returns the in-memory representation of the log stream.
        //
        // Completion status:
        //
        //  STATUS_SUCCESS - The operation completed successfully.
        //
        //  STATUS_NOT_FOUND - The speficied log stream does not exist.
        //
        //  STATUS_DELETE_PENDING - Subject Stream is in the process of being deleted
        //
        virtual VOID
        StartOpenLogStream(
            __in const RvdLogStreamId& LogStreamId,
            __out RvdLogStream::SPtr& LogStream,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) = 0;

    //** Test support API
    virtual BOOLEAN
    IsStreamIdValid(__in RvdLogStreamId StreamId) = 0;

    virtual BOOLEAN
    GetStreamState(
        __in RvdLogStreamId StreamId,
        __out_opt BOOLEAN * const IsOpen = nullptr,
        __out_opt BOOLEAN * const IsClosed = nullptr,
        __out_opt BOOLEAN * const IsDeleted = nullptr) = 0;

    virtual BOOLEAN
    IsLogFlushed() = 0;

    virtual ULONG
    GetNumberOfStreams() = 0;

    virtual ULONG
    GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams) = 0;

    virtual NTSTATUS
    SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal) = 0;
    
