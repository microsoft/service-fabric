// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

public:
    //
    // Factory for creation of RvdLogManager Service instances.
    //
    static NTSTATUS
    Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out RvdLogManager::SPtr& Result);

    //
    // Activate an instance of a RvdLogManager Service.
    //
    // RvdLogManager Service instances that are useful are an KAsyncContext in the operational state.
    // This means that the supplied callback (CallbackPtr) will be invoked once Deactivate() is called
    // and the instance has finished its shutdown process.
    //
    // Returns: STATUS_PENDING - Service successful started. Once this status is returned, that
    //                           instance will accept other operations until Deactive() is called.
    //
    virtual NTSTATUS
    Activate(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    //
    // Trigger shutdown on an active RvdLogManager Service instance.
    //
    // Once called, a Deactivated (or deactivating) RvdLogManager will reject any further operations with
    // a Status or result of K_STATUS_SHUTDOWN_PENDING.
    //
    // Any RvdLog and RvdLogStream instances related to a deactive RvdLogManager will continue to function AND
    // will hold up the shutdown completion callback supplied in Activate() until all application references
    // (i.e. SPtrs) are released to those RvdLogStream and RvdLogManager instances. In other words the application
    // is responsible for shutting down its use of these Log sub-components.
    //
    virtual VOID
    Deactivate() = 0;

    class AsyncEnumerateLogs abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateLogs);

    public:
        // This method asynchronously enumerates all logs on a disk.
        //
        // Parameters:
        //      DiskId              - Supplies KGuid ID of the disk.
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the enumeration is complete.
        // Returns:
        //      LogIdArray          - Array of log IDs on the selected disk.
        //
        // Completion status:
        //      STATUS_SUCCESS      - The operation completed successfully.
        //
        virtual VOID
        StartEnumerateLogs(
            __in const KGuid& DiskId,
            __out KArray<RvdLogId>& LogIdArray,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context)= 0;

    class AsyncCreateLog abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLog);

    public:
        static const int MaxLogTypeLength = 16;

        //
        // Log creation flags
        //
        static const DWORD FlagSparseFile = 1;
        
        //
        // This method asynchronously create a new log.
        // Once it completes, the log container is created and formatted appropriately.
        //
        // Parameters:
        //      DiskId              - Supplies KGuid ID of the disk on which the log will reside.
        //      LogId               - Supplies ID of the log.
        //      LogType             - Supplies the log type string. It
        //                            does not need to unique per log.
        //                            Maximum length is MaxLogTypeLength
        //      LogSize             - Supplies the desired size of the log container in bytes.
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is created on disk.
        //
        // Optional Parameters:
        //      MaxAllowedStreams   - Overrides default maximum number of allowed stream in created log
        //      MaxRecordSize       - Overrides default maximum size of allowed records in created log
        //          NOTE: 0 passed in optional parameters will cause default value to be used
        //      
        //      FullyQualifiedLogFilePath -  Any location can be given in the form of "Vol\<path>\<filename.ext>". The FQN of the 
        //                                   resulting log file will be "\GLOBAL??\Volume{DiskId}\<path>\<filename.ext>". The
        //                                   directories indicated within the supplied path must exist.
        //
        //                                   Also a hardlink alias @ "\GLOBAL??\Volume{DiskId}\RvdLog\Log{LogId}.log" will
        //                                   be created to support log enumeration. It is best practice to use an AsyncDeleteLog
        //                                   operation to remove log files. Removal of "\GLOBAL??\Volume{DiskId}\<path>\<filename.ext>"
        //                                   "\GLOBAL??\Volume{DiskId}\RvdLog\Log{LogId}.log" outside of AsyncDeleteLog will leak 
        //                                   the allocated log file space. Iif "\GLOBAL??\Volume{DiskId}\<path>\<filename.ext>" 
        //                                   is deleted outside of AsyncDeleteLog, the next time any log file creation, open, deletion
        //                                   operation is done on DiskId, such a stranded "\GLOBAL??\Volume{DiskId}\RvdLog\Log{LogId}.log" 
        //                                   will be automatically deleted.
        //
        // Returns:
        //      Log                 - Returns an in-memory representation of the opened (created) log.
        //
        // Completion status:
        //      STATUS_SUCCESS                  - The operation completed successfully.
        //      STATUS_OBJECT_NAME_COLLISION    - A log with the same {DiskId, LogId} already exists.
        //      STATUS_INVALID_PARAMETER        - One of the supplied parameters is invalid
        //
        virtual VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartCreateLog(
            __in KStringView const& FullyQualifiedLogFilePath,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartCreateLog(
            __in KGuid const& DiskId,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __in DWORD Flags,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartCreateLog(
            __in KStringView const& FullyQualifiedLogFilePath,
            __in RvdLogId const& LogId,
            __in KWString& LogType,
            __in LONGLONG LogSize,
            __in ULONG MaxAllowedStreams,
            __in ULONG MaxRecordSize,
            __in DWORD Flags,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    };

    virtual NTSTATUS
    CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context) = 0;

    class AsyncOpenLog abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLog);

    public:
        //
        // This method asynchronously opens an existing log.
        // Once it completes, the returned log object has finished its
        // crash recovery and is ready to use.
        //
        // Parameters:
        //
        //      DiskId              - Supplies KGuid ID of the disk on which the log resides.
        //      LogId               - Supplies ID of the log.
        //      Log                 - Returns an in-memory representation of the opened log.
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is opened and ready to use.
        //
        // Completion status:
        //      STATUS_SUCCESS                  - The operation completed successfully.
        //      STATUS_NOT_FOUND                - The speficied log does not exist.
        //      STATUS_CRC_ERROR                - The log file failed CRC verification.
        //      K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
        //      K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log id is incorrect
        //
        virtual VOID
        StartOpenLog(
            __in const KGuid& DiskId,
            __in const RvdLogId& LogId,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        // This method asynchronously opens an existing log via a fully qualified path and file name.
        // Once this operation completes, the returned log object has finished its crash recovery 
        // and is ready to use.
        //
        // Parameters:
        //
        //      FullyQualifiedLogFilePath - Supplies the fully qualified path and name of the log file
        //      Log                 - Returns an in-memory representation of the opened log.
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is opened and ready to use.
        //
        // Completion status:
        //      STATUS_SUCCESS                  - The operation completed successfully.
        //      STATUS_NOT_FOUND                - The speficied log does not exist.
        //      STATUS_CRC_ERROR                - The log file failed CRC verification.
        //      K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
        //      K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log id is incorrect
        //
        virtual VOID
        StartOpenLog(
            __in const KStringView& FullyQualifiedLogFilePath,
            __out RvdLog::SPtr& Log,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    };
		
    virtual NTSTATUS
    CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context) = 0;

    class AsyncQueryLogId abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncQueryLogId);

    public:
        // This method determines the log id for a log container.
        //
        // Parameters:
        //
        //      FullyQualifiedLogFilePath - Supplies the fully qualified path and name of the log file
        //      LogId               - Returns the log id of the log
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is opened and ready to use.
        //
        // Completion status:
        //      STATUS_SUCCESS                  - The operation completed successfully.
        //      STATUS_NOT_FOUND                - The speficied log does not exist.
        //      STATUS_CRC_ERROR                - The log file failed CRC verification.
        //      K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
        //      K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log id is incorrect
        //
        virtual VOID
        StartQueryLogId(
            __in const KStringView& FullyQualifiedLogFilePath,
            __out RvdLogId& LogId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
		
    };
		
    virtual NTSTATUS
    CreateAsyncQueryLogIdContext(__out AsyncQueryLogId::SPtr& Context) = 0;
	
    class AsyncDeleteLog abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLog);

    public:
        //
        // This method asynchronously deletes an existing log.
        //
        // Parameters:
        //
        //      DiskId              - Supplies KGuid ID of the disk on which the log resides.
        //      LogId               - Supplies ID of the log.
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is deleted.
        //
        // Completion status:
        //      STATUS_SUCCESS      - The operation completed successfully.
        //      STATUS_NOT_FOUND    - The speficied log does not exist.
        //

        virtual VOID
        StartDeleteLog(
            __in const KGuid& DiskId,
            __in const RvdLogId& LogId,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        // This method asynchronously deletes an existing log.
        //
        // Parameters:
        //
        //      FullyQualifiedLogFilePath - Supplies the fully qualified path and name of the log file
        //      ParentAsyncContext  - Supplies an optional parent async context.
        //      CallbackPtr         - Supplies an optional callback delegate to be notified
        //                            when the log is opened and ready to use.
        //
        // Completion status:
        //      STATUS_SUCCESS                  - The operation completed successfully.
        //      STATUS_NOT_FOUND                - The speficied log does not exist.
        //      STATUS_CRC_ERROR                - The log file failed CRC verification.
        //      K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
        //      K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log id is incorrect
        //
        virtual VOID
        StartDeleteLog(
            __in const KStringView& FullyQualifiedLogFilePath,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context) = 0;

    //
    // Callback routine for verifying data integrity of a log record that has an IoBuffer.
    // It returns TRUE if the log record buffers are valid, FALSE otherwise.
    //
    // Parameters:
    //      MetaDataBuffer/Size - Supplies the meta data buffer block of the record.
    //      IoBuffer            - Supplies the IoBuffer part of the record.
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Log record verified as correct
    //      K_STATUS_LOG_STRUCTURE_FAULT    - Log record was incorrect - this is a normal
    //                                        indication and if returned during log recovery,
    //                                        the recovery process will continue.
    //      !NT_SUCCESS()                   - All other failure status' will cause the related
    //                                        operation (e.g. the recovery process) to fail with
    //                                        this returned status.
    //
typedef KDelegate<NTSTATUS(
    __in UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
)> VerificationCallback;


    //
    // This method registers a log record verification callback for a type of log stream.
    //
    // Parameters:
    //      LogStreamType   - Supplies the type of the log stream.
    //      Callback        - Supplies the callback delegate.
    //
    virtual NTSTATUS
    RegisterVerificationCallback(
        __in const RvdLogStreamType& LogStreamType,
        __in VerificationCallback Callback) = 0;

    //
    // This method unregisters a log record verification callback for a type of log stream.
    // It returns the registered callback delegate.
    //
    // Parameters:
    //      LogStreamType - Supplies the type of the log stream.
    //
    virtual VerificationCallback
    UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType) = 0;

    //
    // This method queries the log record verification callback for a type of log stream.
    // If there is no such callback, the returned delegate object is null.
    //
    // Parameters:
    //      LogStreamType - Supplies the type of the log stream.
    //
    virtual VerificationCallback
    QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType) = 0;
