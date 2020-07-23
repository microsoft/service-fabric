// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    GlobalWString EseInstance::LocalBackupDirectorySuffix = make_global<std::wstring>(L"BAK");
    GlobalWString EseInstance::RestoreDirectorySuffix = make_global<std::wstring>(L"RES");
    GlobalWString EseInstance::CompactionFileExtension = make_global<std::wstring>(L"cmp");

	/// <summary>
	/// The directory prefix for storing truncated ESE backup logs.
	/// </summary>
	GlobalWString EseInstance::TempTruncateDirectoryPrefix = make_global<std::wstring>(L"TRN");

    Global<RwLock> EseInstance::sLock_ = make_global<RwLock>();
    Global<EseInstance::InstanceMap> EseInstance::eseInstances_ = make_global<InstanceMap>();

    StringLiteral const TraceComponent("EseInstance");

    EseInstance::EseInstance(
        EseLocalStoreSettingsSPtr const & settings,
        EseStorePerformanceCountersSPtr const & counters,
        std::wstring const & instanceName,
        std::wstring const & eseFilename,
        std::wstring const & eseDirectory,
        bool isRestoreValidation)
        : instanceId_(JET_instanceNil),
          thisLock_(),
          settings_(settings),
          perfCounters_(counters),
          instanceName_(instanceName),
          eseFilename_(eseFilename),
          eseDirectory_(eseDirectory),
          restoreDirectory_(EseInstance::GetRestoreDirectoryName(eseDirectory)),
          isRestoreValidation_(isRestoreValidation),
          attachedDatabases_(),
          openFileSize_(0),
          activeSessions_(),
          instanceIdRef_(0),
          aborting_(false),
          isHealthy_(true),
          allEseCallsCompleted_(false),
          allEseCallbacksCompleted_(false),
          pendingCommits_(),
          nextBatchStartCommitId_(0),
          databasePageSize_(0),
          logFileSizeInKB_(0),
          minCacheSize_(0)
    {
    }

    EseInstance::~EseInstance()
    {
        Abort();
    }

    EseInstanceSPtr EseInstance::CreateSPtr(
        EseLocalStoreSettingsSPtr const & settings,
        EseStorePerformanceCountersSPtr const & counters,
        std::wstring const & instanceName,
        std::wstring const & eseFilename,
        std::wstring const & eseDirectory,
        bool isRestoreValidation)
    {
        return shared_ptr<EseInstance>(new EseInstance(
            settings, 
            counters,
            instanceName,
            eseFilename,
            eseDirectory,
            isRestoreValidation));
    }

    ErrorCode EseInstance::DoCompaction()
    {
        auto srcFile = eseFilename_;
        auto srcPath = Path::Combine(eseDirectory_, srcFile);
        auto destFile = wformatString("{0}.{1}", eseFilename_, *CompactionFileExtension);
        auto destPath = Path::Combine(eseDirectory_, destFile);
        
        if (settings_->CompactionThresholdInMB <= 0)
        {
            WriteInfo(
                TraceComponent, 
                "{0} auto-compaction disabled for {1}",
                this->TraceId,
                srcPath);

            return File::GetSize(srcPath, openFileSize_);
        }

        if (File::Exists(destPath))
        {
            auto error = File::Delete2(destPath);
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent, 
                    "{0} failed to cleanup {1}: {2}",
                    this->TraceId,
                    destPath,
                    error);

                return error;
            }
        }

        int64 preFileSize = 0;
        auto error = File::GetSize(srcPath, preFileSize);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent, 
                "{0} failed to get pre-compaction file size {1}: {2}",
                this->TraceId,
                srcPath,
                error);

            return error;
        }

        if (!this->ShouldAutoCompact(preFileSize))
        {
            WriteInfo(
                TraceComponent, 
                "{0} skipping auto-compaction {1}: size={2}bytes threshold={3}MB",
                this->TraceId,
                srcPath,
                preFileSize,
                settings_->CompactionThresholdInMB);

            return ErrorCodeValue::Success;
        }

        auto session = EseSession::CreateSPtr(shared_from_this());
        error = this->JetToErrorCode(session->Initialize());
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to create session {1}: {2}",
                this->TraceId,
                srcPath,
                error);

            return error;
        }

        error = this->JetToErrorCode(this->AttachDatabase(*session, srcPath, true, JET_bitDbReadOnly));
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to attach database {1}: {2}",
                this->TraceId,
                srcPath,
                error);

            return error;
        }

        WriteInfo(
            TraceComponent, 
            "{0} auto-compacting {1}->{2}: size={3}bytes threshold={4}MB",
            this->TraceId,
            srcPath,
            destPath,
            preFileSize,
            settings_->CompactionThresholdInMB);

        auto jetError = CALL_ESE_NOTHROW(
            JetCompact(
                session->SessionId,
                srcPath.c_str(),
                destPath.c_str(),
                NULL, // pfnStatus
                NULL, // pconvert
                0)); // grbit
        error = this->JetToErrorCode(jetError);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to auto-compact {1}: {2}",
                this->TraceId,
                srcPath,
                error);

            // continue to detach database
        }

        auto detachError = this->JetToErrorCode(this->DetachDatabase(*session, srcPath));
        if (!detachError.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to detach database {1}: {2}",
                this->TraceId,
                srcPath,
                detachError);

            return ErrorCode::FirstError(error, detachError);
        }

        if (!error.IsSuccess()) { return error; }

        session.reset();

        error = File::MoveTransacted(destPath, srcPath, true); // overwrite
        if (error.IsSuccess())
        {
            int64 postFileSize = 0;
            error = File::GetSize(srcPath, postFileSize);
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent, 
                    "{0} failed to get post-compaction file size {1}: {2}",
                    this->TraceId,
                    srcPath,
                    error);

                return error;
            }

            WriteInfo(
                TraceComponent, 
                "{0} compacted database {1}->{2}: old={3}bytes new={4}bytes",
                this->TraceId,
                destPath,
                srcPath,
                preFileSize,
                postFileSize);

            openFileSize_ = postFileSize;
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to move compacted database {1}->{2}: {3}",
                this->TraceId,
                destPath,
                srcPath,
                error);
        }

        return error;
    }

    ErrorCode EseInstance::PrepareForRestore(const std::wstring & localBackupDirectory)
    {
        ErrorCode error = ErrorCode::Success();

        if (Directory::Exists(restoreDirectory_) && Directory::Exists(eseDirectory_))
        {
            // If the local backup already exists, then an earlier restore
            // attempt has taken place and we failed to either remove or
            // replace the local backup. Delete it now to allow
            // retry of restore.
            //
            if (Directory::Exists(localBackupDirectory))
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} Local backup already exists - deleting it: '{1}'",
                    this->TraceId,
                    localBackupDirectory);

                error = Directory::Delete(localBackupDirectory, true);

                if (error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent, 
                        "{0} Successfully deleted existing local backup '{1}'",
                        this->TraceId,
                        localBackupDirectory);
                }
                else
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} Failed to delete existing local backup '{1}': {2}",
                        this->TraceId,
                        localBackupDirectory,
                        error);

                    return error;
                }
            }

            // JetRestoreInstance protects against restoring over an
            // existing database and unintentionally losing data.
            // Move the current database directory to a local backup and 
            // try to move it back if the restore fails.
            // There is no guarantee we can move it back successfully.
            //
            // Some checks are performed at the ReplicatedStore level to
            // prevent misuse of restore functionality.
            //
            error = Directory::Rename(eseDirectory_, localBackupDirectory, true); // overwrite

            if (error.IsSuccess()) 
            { 
                error = Directory::Create2(eseDirectory_);
            }

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} Successfully created local backup '{1}' for restore",
                    this->TraceId,
                    localBackupDirectory);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} Failed to create local backup '{1}' for restore: {2}",
                    this->TraceId,
                    localBackupDirectory,
                    error);

                return error;
            }
        }

        return error;
    }

    /// <summary>
    /// Sets the opening JET system parameters.
    /// </summary>
    /// <returns>
    /// ErrorCode::Success() if all system parameters were set correctly. An appropriate 
    /// error code value otherwise.
    /// </returns>
    ErrorCode EseInstance::SetOpeningParameters()
    {
        ErrorCode error = ErrorCode::Success();

        auto jetError = CALL_ESE_CHECK_NOTHROW(
            &EseException::ExpectSuccessOrAlreadyInitialized,
            this->SetSystemParameter(JET_paramMaxInstances, StoreConfig::GetConfig().MaxJetInstances));

        if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
        {
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            error = this->ComputeDatabasePageSize();
        }

        if (error.IsSuccess())
        {
            error = this->ComputeLogFileSize();
        }

        if (error.IsError(ErrorCodeValue::StoreInUse) && StoreConfig::GetConfig().AssertOnOpenSharingViolation)
        {
            // JetGetDatabaseFileInfo will return JET_errFileAccessDenied on re-open if the database hasn't been
            // released for whatever reason. Avoid blocking the re-open indefinitely and self-mitigate by restarting
            // the process.
            //
            Assert::CodingError("{0} Configuration ReplicatedStore/AssertOnOpenSharingViolation=true: asserting on StoreInUse", this->TraceId);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccessOrAlreadyInitialized,
                this->SetSystemParameter(JET_paramDatabasePageSize, databasePageSize_));

            if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
            {
                error = this->JetToErrorCode(jetError);
            }
        }    

        if (error.IsSuccess())
        {
            if (settings_->MaxCacheSizeInMB != Constants::UseEseDefaultValue)
            {
                // Best effort to match requested cache size limit in MB. However, database page size is per instance, so the
                // actual limit could be different than expected. This mainly occurs when upgrading from an older version of the product in
                // which the default database page size was smaller.
                //
                jetError = CALL_ESE_CHECK_NOTHROW(
                    &EseException::ExpectSuccessOrAlreadyInitialized,
                    this->SetSystemParameter(
                        JET_paramCacheSizeMax, this->ConvertToDatabasePages(settings_->MaxCacheSizeInMB)));

                if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
                {
                    error = this->JetToErrorCode(jetError);
                }
            }
        }

        if (error.IsSuccess())
        {
            if (settings_->MinCacheSizeInMB != Constants::UseEseDefaultValue)
            {
                jetError = CALL_ESE_CHECK_NOTHROW(
                    &EseException::ExpectSuccessOrAlreadyInitialized,
                    this->SetSystemParameter(
                        JET_paramCacheSizeMin, this->ConvertToDatabasePages(settings_->MinCacheSizeInMB)));

                if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
                {
                    error = this->JetToErrorCode(jetError);
                }
            }
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(
                JetCreateInstance(
                    &instanceId_,
                    instanceName_.c_str()));

            if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
            {
                error = this->JetToErrorCode(jetError);
            }
        }    

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccessOrAlreadyInitialized,
                this->SetSystemParameter(JET_paramVerPageSize, StoreConfig::GetConfig().VerPageSizeInKB * 1024));

            if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
            {
                error = this->JetToErrorCode(jetError);
            }
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramDurableCommitCallback, (JET_API_PTR)EseInstance::CommitCallback));

            if (!EseException::ExpectSuccessOrAlreadyInitialized(jetError))
            {
                error = this->JetToErrorCode(jetError);
            }
        }

        // The next 4 settings are to catch bugs with leaking jet resources        
        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramMaxSessions, StoreConfig::GetConfig().MaxSessions));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramMaxOpenTables, StoreConfig::GetConfig().MaxOpenTables));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramMaxCursors, settings_->MaxCursors));
            error = this->JetToErrorCode(jetError);
        }

        // Long running transactions cause version pages to grow. Given WF uses short transactions this should not happen.
        // This setting is to catch any bugs that may cause long transactions
        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramMaxVerPages, settings_->MaxVerPages));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramLogFileSize, logFileSizeInKB_));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            // Ese log buffer size is specified in multiples of 512
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramLogBuffers, 2 * settings_->LogBufferSizeInKB));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            // Ese log buffer size is specified in multiples of 512
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramCacheSizeMin , StoreConfig::GetConfig().EseMinDatabasePagesInBufferPoolCache));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramCircularLog, settings_->IsIncrementalBackupEnabled ? 0 : 1));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramCreatePathIfNotExist, 1));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramEnableIndexChecking, 1));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramEnableIndexCleanup, 1));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess() && eseDirectory_.size() > Constants::MaxEseDirectoryLength)
        {
            error = ErrorCode(ErrorCodeValue::PathTooLong, wformatString(GET_STORE_RC(Path_Too_Long), eseDirectory_));
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramSystemPath, eseDirectory_));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramLogFilePath, eseDirectory_));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramEnableDBScanSerialization, StoreConfig::GetConfig().EseEnableScanSerialization ? 1 : 0));
            error = this->JetToErrorCode(jetError);
        }

        if (error.IsSuccess())
        {
            if (StoreConfig::GetConfig().EseScanThrottleInMilleseconds != Constants::UseEseDefaultValue)
            {
                jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramDbScanThrottle, StoreConfig::GetConfig().EseScanThrottleInMilleseconds));
                error = this->JetToErrorCode(jetError);
            }
        }

        if (error.IsSuccess())
        {
            if (StoreConfig::GetConfig().EseScanIntervalMinInSeconds != Constants::UseEseDefaultValue)
            {
                jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramDbScanIntervalMinSec, StoreConfig::GetConfig().EseScanIntervalMinInSeconds));
                error = this->JetToErrorCode(jetError);
            }
        }

        if (error.IsSuccess())
        {
            if (StoreConfig::GetConfig().EseScanIntervalMaxInSeconds != Constants::UseEseDefaultValue)
            {
                jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramDbScanIntervalMaxSec, StoreConfig::GetConfig().EseScanIntervalMaxInSeconds));
                error = this->JetToErrorCode(jetError);
            }
        }

        if (error.IsSuccess())
        {
            jetError = CALL_ESE_NOTHROW(this->SetSystemParameter(JET_paramDefragmentSequentialBTrees, true));
            error = this->JetToErrorCode(jetError);
        }

        return error;
    }

    ErrorCode EseInstance::ComputeDatabasePageSize()
    {
        databasePageSize_ = settings_->DatabasePageSizeInKB * 1024;

        auto filepath = Path::Combine(restoreDirectory_, eseFilename_);
        if (!File::Exists(filepath))
        {
            filepath = Path::Combine(eseDirectory_, eseFilename_);
        }

        if (!File::Exists(filepath))
        {
            return ErrorCodeValue::Success;
        }

        auto retryCount = StoreConfig::GetConfig().OpenDatabaseRetryCount;
        while (retryCount-- >= 0)
        {
            ULONG existingDatabasePageSize = 0;

            auto jetError = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccess,
                JetGetDatabaseFileInfo(
                    filepath.c_str(),
                    &existingDatabasePageSize,
                    sizeof(existingDatabasePageSize),
                    JET_DbInfoPageSize));

            if (jetError == JET_errFileAccessDenied && retryCount > 0)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} Retrying JetGetDatabaseFileInfo({1}) on {2}: retry={3}",
                    this->TraceId,
                    filepath,
                    jetError,
                    retryCount);

                Sleep(StoreConfig::GetConfig().OpenDatabaseRetryDelayMilliseconds);
            }
            else if (!EseException::ExpectSuccess(jetError))
            {
                return this->JetToErrorCode(jetError);
            }
            else if (existingDatabasePageSize != databasePageSize_)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} Database page size mismatch: config={1} existing={2}", 
                    this->TraceId,
                    databasePageSize_, 
                    existingDatabasePageSize);

                databasePageSize_ = existingDatabasePageSize;

                break;
            }
        }
        
        return ErrorCodeValue::Success;
    }

    ErrorCode EseInstance::ComputeLogFileSize()
    {
        logFileSizeInKB_ = settings_->LogFileSizeInKB;

        auto path = Path::Combine(eseDirectory_, StoreConfig::GetConfig().ComputeLogFileSizeSource);

        int64 existingLogFileSize;
        auto error = File::GetSize(path, existingLogFileSize);
        if (!error.IsSuccess()) { return error; }

        if (existingLogFileSize <= 0 || existingLogFileSize % 1024 != 0)
        {
            WriteInfo(
                TraceComponent, 
                "{0} Invalid existing log file size {1} bytes. Keeping config={2}KB",
                this->TraceId,
                existingLogFileSize,
                logFileSizeInKB_);

            return ErrorCodeValue::Success;
        }

        auto existingLogFileSizeInKB = (existingLogFileSize / 1024);

        if (static_cast<int64>(logFileSizeInKB_) != existingLogFileSizeInKB)
        {
            WriteInfo(
                TraceComponent, 
                "{0} Database logfile size (KB) mismatch: config={1} existing={2}", 
                this->TraceId,
                logFileSizeInKB_, 
                existingLogFileSizeInKB);

            logFileSizeInKB_ = existingLogFileSizeInKB;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode EseInstance::DoRestore(const std::wstring & localBackupDirectory)
    {
        ErrorCode error = ErrorCode::Success();

        if (!Directory::Exists(restoreDirectory_))
        {
            WriteNoise(
                TraceComponent,
                "{0} Directory '{1}' to restore instance {2} from does not exist. So, nothing to restore.",
                this->TraceId,
                restoreDirectory_,
                instanceId_);

            return error;
        }
       
        auto jetError = CALL_ESE_NOTHROW(
            JetRestoreInstance(
                instanceId_,                
                restoreDirectory_.c_str(),                
                eseDirectory_.c_str(),
                NULL));

        if (jetError == JET_errSuccess)
        {
            WriteInfo(
                TraceComponent, 
                "{0} Successfully restored instance {1} from '{2}'", 
                this->TraceId,
                instanceId_, 
                restoreDirectory_);

            // During Restore Validation:
            // Leave the validated restore directory. It will be moved to
            // the actual restore location for execution of the actual
            // restore.
            //
            // The local backup did not contain any real data during
            // validation, so delete it.
            //
            // During Restore:
            // Delete both the local backup and the (no longer needed)
            // restore directory. Avoid having the administrator manually 
            // delete the local backup when the restore succeeds.
            //
            vector<wstring> directoriesToCleanup;
            directoriesToCleanup.push_back(localBackupDirectory);

            if (!isRestoreValidation_)
            {
                directoriesToCleanup.push_back(restoreDirectory_);
            }

            for (auto const & directoryToCleanup : directoriesToCleanup)
            {
                error = Directory::Delete(directoryToCleanup, true);

                if (error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent, 
                        "{0} Successfully deleted after restore '{1}'",
                        this->TraceId,
                        directoryToCleanup);
                }
                else
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} Failed to delete after restore '{1}': {2}",
                        this->TraceId,
                        directoryToCleanup,
                        error);
                }
            }
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to restore instance {1} from '{2}': {3}", 
                this->TraceId,
                instanceId_, 
                restoreDirectory_,
                jetError);

            // Do not set instanceId_ to JET_instanceNil here since OnOpen()
            // will fail and cause FabricComponent to call OnAbort(). That
            // will in turn result in a call to JetTerm(), which is needed
            // if JetCreateInstance() was ever called.

            error = this->JetToErrorCode(jetError);

            // Best-effort recovery from failed restore
            //
            auto innerError = Directory::Rename(localBackupDirectory, eseDirectory_, true); //overwrite

            if (innerError.IsSuccess())
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} Successfully recovered local backup '{1}' to '{2}'",
                    this->TraceId,
                    localBackupDirectory,
                    eseDirectory_);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} Failed to recover local backup '{1}' to '{2}': {3}",
                    this->TraceId,
                    localBackupDirectory,
                    eseDirectory_,
                    innerError);
            }
        }
        
        return error;
    }

    ErrorCode EseInstance::PrepareDatabaseForUse()
    {
        auto jetError = CALL_ESE_NOTHROW(JetInit(&instanceId_));

        auto error = (jetError == JET_errInvalidParameter) 
            ? ErrorCode(ErrorCodeValue::PathTooLong, wformatString(GET_STORE_RC(Path_Too_Long), eseDirectory_))
            : this->JetToErrorCode(jetError);

        if (error.IsSuccess())
        {
            WriteNoise(TraceComponent, "{0} Initialized instance {1}", this->TraceId, instanceId_);
        }
        else
        {
            // JetTerm can only be called on successful JetInit.
            instanceId_ = JET_instanceNil;
        }

        if (error.IsSuccess())
        {
            AcquireWriteLock acquire(*EseInstance::sLock_);

            auto result = eseInstances_->insert(make_pair(instanceId_, shared_from_this()));

            ASSERT_IFNOT(result.second, "{0} Duplicate ESE instance id {1}", this->TraceId, instanceId_);
        }

        return error;
    }

    /// <summary>
    /// Performs initialization functions like preparing the ESE database for first use, setting system
    /// parameters required for ESE etc.
    /// </summary>
    /// <returns>ErrorCode::Success() if the initialization was successful. An appropriate error code otherwise.</returns>
    ErrorCode EseInstance::OnOpen()
    {
        ASSERT_IF(instanceId_ != JET_instanceNil, "{0} Cannot initialize twice", this->TraceId);
        
        Stopwatch stopwatch;
        stopwatch.Start();

        auto localBackupDirectory = GetLocalBackupDirectoryName(eseDirectory_);

        auto error = PrepareForRestore(localBackupDirectory);
        if (!error.IsSuccess()) { return error; }

        error = SetOpeningParameters();
        if (!error.IsSuccess()) { return error; }

        error = DoRestore(localBackupDirectory);
        if (!error.IsSuccess()) { return error; }

        auto elapsedRestore = stopwatch.Elapsed;

        error = PrepareDatabaseForUse();
        if (!error.IsSuccess()) { return error; }

        auto elapsedInit = stopwatch.Elapsed;

        error = DoCompaction();
        if (!error.IsSuccess()) { return error; }

        auto elapsedCompaction = stopwatch.Elapsed;

        WriteInfo(
            TraceComponent, 
            "{0} EseInstance open complete: error={1} create/restore={2}ms init={3}ms compact={4}ms", 
            this->TraceId,
            error, 
            elapsedRestore.TotalPositiveMilliseconds(),
            elapsedInit.TotalPositiveMilliseconds(),
            elapsedCompaction.TotalPositiveMilliseconds());

        return error;
    }

    void EseInstance::OnAbort()
    {
        // We only have one kind of shutdown.
        // Either way, we rollback all active transactions but do an orderly ESE shutdown, and
        // then fail all subsequent calls.
        OnClose();
    }

    ErrorCode EseInstance::OnClose()
    {
        {
            AcquireWriteLock acquire(thisLock_);

            // Multiple calls to OnAbort should result in no-op for all except first calls
            if (instanceId_ == JET_instanceNil || aborting_)
            {
                return ErrorCodeValue::Success;
            }
            aborting_ = true;

            if (0 == instanceIdRef_)
            {
                allEseCallsCompleted_.Set();
            }
        }

        Stopwatch stopwatch;
        stopwatch.Restart();

        WriteInfo(TraceComponent, "{0} ({1}) aborting backups", this->TraceId, instanceId_);

        // Best-effort abort of pending backups. Let the macro trace the
        // error (if any) but don't process the return code.
        //
        CALL_ESE_NOTHROW(JetStopBackupInstance(instanceId_));

        WriteInfo(
            TraceComponent,
            "{0} ({1}) waiting for ESE calls to complete: elapsed={2}",
            this->TraceId,
            instanceId_,
            stopwatch.Elapsed);

        // Wait for all calls to ESE not holding the lock to not deadlock. At this point there could
        // be calls to ESE on other threads. If so they would take a read lock on thisLock_ to
        // decrement instanceIdRef_ on return from ESE.
        //
        // It is ok to do these checks outside of the lock since once
        // aborting_ is set, ref count can't increase
        //
        allEseCallsCompleted_.WaitOne();

        WriteInfo(
            TraceComponent,
            "{0} ({1}) aborting active sessions: elapsed={2}",
            this->TraceId,
            instanceId_,
            stopwatch.Elapsed);

        // Calling abort on ESE sessions may trigger JetRollback(), which internally seems to wait 
        // for pending ESE commit callbacks: CommitCallback(), which in turn needs to acquire thisLock_
        // in OnCommitComplete() to preserve ordered completion.
        //
        // Abort outside thisLock_, the list of active sessions is already gated by aborting_.
        //
        AbortActiveSessions();

        bool createDummyTransaction = false;
        {
            AcquireWriteLock acquire(thisLock_);

            if (pendingCommits_.empty())
            {
                allEseCallbacksCompleted_.Set();
            }
            else
            {
                createDummyTransaction = true;
            }
        }

        // Create dummy transaction to make all pending async commits to complete quickly
        if (createDummyTransaction)
        {
            JET_SESID sessionId;
            if (JET_errSuccess == JetBeginSession(instanceId_, &sessionId, NULL, NULL))
            {
                JetCommitTransaction(sessionId, JET_bitWaitAllLevel0Commit);
                JetEndSession(sessionId, NULL);
            }
        }

        WriteInfo(
            TraceComponent,
            "{0} ({1}) waiting for ESE commits to complete: elapsed={2}",
            this->TraceId,
            instanceId_,
            stopwatch.Elapsed);

        // Wait for last call to ESE to return, and all async commits to be done. That will trigger abort
        allEseCallbacksCompleted_.WaitOne();

        ASSERT_IF(
            0 != instanceIdRef_ || !pendingCommits_.empty(),
            "{0} Outstanding ESE work still exists: references={1} pending commits={2}.", 
            this->TraceId,
            instanceIdRef_, 
            pendingCommits_.size());

        {
            AcquireWriteLock acquire(*EseInstance::sLock_);

            EseInstance::eseInstances_->erase(instanceId_);
        }

        WriteInfo(TraceComponent, "{0} ({1}) calling JetTerm: elapsed={2}", this->TraceId, instanceId_, stopwatch.Elapsed);

        auto termError = this->TerminateWithRetries();

        WriteInfo(TraceComponent, "{0} ({1}) closed: error={2} elapsed={3}", this->TraceId, instanceId_, termError, stopwatch.Elapsed);

        instanceId_ = JET_instanceNil;

        return termError;
    }

    void EseInstance::AbortActiveSessions()
    {
        ASSERT_IF(
            instanceId_ == JET_instanceNil || !aborting_,
            "{0} AbortActiveSessions called with instance id {1}, aborting_ {2}.", this->TraceId, instanceId_, static_cast<bool>(aborting_));

        // Need to abort here since at this point no sessions can be in the middle of a commit (or any other ESE) call 
        //
        for (auto const & it : activeSessions_)
        {
            it.second->Abort();
        }

        activeSessions_.clear();
    }

    ErrorCode EseInstance::TerminateWithRetries()
    {
        ErrorCode termError;
        JET_GRBIT termFlag = JET_bitTermComplete;

        for (;;)
        {
            // JET_bitTermStopBackup is technically not needed at this point, since all
            // ESE activity should have completed already
            //
            auto jetErr = CALL_ESE_NOTHROW(JetTerm2(instanceId_, (termFlag | JET_bitTermStopBackup)));

            termError = this->JetToErrorCode(jetErr);

            if (termError.IsError(ErrorCodeValue::StoreInUse) && termFlag != JET_bitTermDirty)
            {
                // MSDN says that JetTerm can only return JET_errTooManyActiveUsers when using
                // JET_bitTermComplete, but that doesn't seem to be true. Try escalating to
                // JET_bitTermDirty if JET_bitTermAbrupt still fails.
                //
                switch (termFlag)
                {
                case JET_bitTermComplete:
                    termFlag = JET_bitTermAbrupt;
                    break;
                case JET_bitTermAbrupt:
                    termFlag = JET_bitTermDirty;
                    break;
                };

                WriteInfo(TraceComponent, "{0} ({1}) retrying JetTerm: flag={2} error={3}", this->TraceId, instanceId_, termFlag, termError);
                Sleep(StoreConfig::GetConfig().DeleteDatabaseRetryDelayMilliseconds);
            }
            else
            {
                break;
            }
        }

        switch (termError.ReadValue())
        {
        case ErrorCodeValue::Success:
        case ErrorCodeValue::StoreInUse:
        case ErrorCodeValue::StoreFatalError:
        case ErrorCodeValue::DatabaseFilesCorrupted:
        case ErrorCodeValue::ObjectClosed:
            break;
        default:
            Assert::CodingError("{0} JetTerm failed: error={1}", this->TraceId, termError);
        }

        return termError;
    }

    JET_ERR EseInstance::AttachDatabase(
        EseSession const & session,
        std::wstring const & filename,
        bool callJetAttach,
        JET_GRBIT flags)
    {
        ASSERT_IF(instanceId_ == JET_instanceNil, "{0} instanceId_ == JET_instanceNil", this->TraceId);

        JET_ERR jetErr = JET_errSuccess;

        auto iter = attachedDatabases_.find(filename);
        if (iter != attachedDatabases_.end())
        {
            ++(iter->second);
        }
        else
        {
            if (callJetAttach)
            {
                jetErr = CALL_ESE_NOTHROW(
                    JetAttachDatabase2(
                    session.SessionId,
                    filename.c_str(),
                    0,
                    flags == 0 ? GetOptions() : flags));
            }

            if (JET_errSuccess == jetErr)
            {
                AcquireWriteLock acquire(thisLock_);
                attachedDatabases_.insert(std::pair<std::wstring, int>(filename, 1));
            }
        }

        return jetErr;
    }

    JET_ERR EseInstance::DetachDatabase(
        EseSession const & session,
        std::wstring const & filename)
    {
        ASSERT_IF(instanceId_ == JET_instanceNil, "{0} instanceId_ == JET_instanceNil", this->TraceId);

        JET_ERR jetErr = JET_errSuccess;

        bool callJetDetach = false;
        {
            AcquireWriteLock acquire(thisLock_);

            auto iter = attachedDatabases_.find(filename);
            ASSERT_IF(iter == attachedDatabases_.end(), "{0} Detaching database that was not attached.", this->TraceId);

            if (--(iter->second) == 0)
            {
                callJetDetach = true;
                attachedDatabases_.erase(iter);

            }
        }

        if (callJetDetach)
        {            
            jetErr = CALL_ESE_NOTHROW(
                JetDetachDatabase(
                session.SessionId,
                filename.c_str()));
        }

        return jetErr;
    }

    JET_GRBIT EseInstance::GetOptions()
    {
        JET_GRBIT grbit = 0;

        if (StoreConfig::GetConfig().EseEnableBackgroundMaintenance)
        {
            grbit |= JET_bitDbEnableBackgroundMaintenance;
        }

        return grbit;
    }

    ErrorCode EseInstance::ForceMaxCacheSize()
    {
        JET_API_PTR currentCacheSize = 0;

        auto targetCacheSize = this->ConvertToDatabasePages(settings_->MaxCacheSizeInMB);

        auto jetError = CALL_ESE_NOTHROW(
            JetGetSystemParameter(
                instanceId_,
                JET_sesidNil,
                JET_paramCacheSize,
                &currentCacheSize,
                NULL,
                0));

        if (!EseException::ExpectSuccess(jetError))
        {
            return this->JetToErrorCode(jetError);
        }

        jetError = CALL_ESE_CHECK_NOTHROW(
            &EseException::ExpectSuccess,
            this->SetSystemParameter(JET_paramCacheSize, targetCacheSize));

        if (!EseException::ExpectSuccess(jetError))
        {
            return this->JetToErrorCode(jetError);
        }

        jetError = CALL_ESE_NOTHROW(
            JetGetSystemParameter(
                instanceId_,
                JET_sesidNil,
                JET_paramCacheSizeMin,
                &minCacheSize_,
                NULL,
                0));

        if (!EseException::ExpectSuccess(jetError))
        {
            return this->JetToErrorCode(jetError);
        }

        jetError = CALL_ESE_CHECK_NOTHROW(
            &EseException::ExpectSuccess,
            this->SetSystemParameter(JET_paramCacheSizeMin, targetCacheSize));

        if (EseException::ExpectSuccess(jetError))
        {
            WriteInfo(
                TraceComponent, 
                "{0} ForceMaxCacheSize: target={1} current={2} min={3}", 
                this->TraceId,
                targetCacheSize, 
                currentCacheSize,
                minCacheSize_);
        }

        return this->JetToErrorCode(jetError);
    }

    ErrorCode EseInstance::ResetCacheSize()
    {
        if (minCacheSize_ == 0)
        {
            WriteInfo(TraceComponent, "{0} Skipping ResetCacheSize", this->TraceId);

            return ErrorCodeValue::Success;
        }

        auto targetCacheSize = 0; // dynamic

        auto jetError = CALL_ESE_CHECK_NOTHROW(
            &EseException::ExpectSuccess,
            this->SetSystemParameter(JET_paramCacheSize, targetCacheSize));

        if (!EseException::ExpectSuccess(jetError))
        {
            return this->JetToErrorCode(jetError);
        }

        jetError = CALL_ESE_CHECK_NOTHROW(
            &EseException::ExpectSuccess,
            this->SetSystemParameter(JET_paramCacheSizeMin, minCacheSize_));

        if (EseException::ExpectSuccess(jetError))
        {
            WriteInfo(TraceComponent, "{0} ResetCacheSize: target={1} min={2}", this->TraceId, targetCacheSize, minCacheSize_);
        }

        return this->JetToErrorCode(jetError);
    }

    ErrorCode EseInstance::JetToErrorCode(JET_ERR jetError)
    {
        auto error = ErrorCode(EseError::GetErrorCode(jetError).ReadValue(), wformatString("JET_ERR={0}", jetError));

        if (error.IsError(ErrorCodeValue::StoreFatalError) && settings_->AssertOnFatalError)
        {
            Assert::CodingError("{0} Configuration EseStore/AssertOnFatalError=true: asserting on store fatal error", this->TraceId);
        }

        return error;
    }

    bool EseInstance::ShouldAutoCompact(int64 fileSize)
    {
        return ShouldAutoCompact(fileSize, settings_);
    }

    bool EseInstance::ShouldAutoCompact(int64 fileSize, EseLocalStoreSettingsSPtr const & settings)
    {
        auto threshold = settings->CompactionThresholdInMB;
        return (threshold > 0 && (fileSize >> 20) >= threshold);
    }

    /// <summary>
    /// Invokes the JetBackupInstance API. Only incremental or atomic (full) backup are supported currently.
    /// For incremental backup, JET_paramCircularLog should be turned off.
    /// </summary>
    /// <param name="dir">The destination directory for the backup.</param>
    /// <param name="jetBackupOptions">The backup options to be passed to JetBackupInstance.
    /// Currently, only JET_bitBackupIncremental or JET_bitBackupAtomic are supported.</param>	
    JET_ERR EseInstance::Backup(std::wstring const & dir, LONG jetBackupOptions)
    {				
        if (!(jetBackupOptions == JET_bitBackupIncremental || jetBackupOptions == JET_bitBackupAtomic))
        {
            // Even though the options are flags and could probably be used in combinations, 
            // we currently support only these 2 explicit cases (incremental or atomic)
            WriteWarning(
                TraceComponent,
                "{0} Invalid jetBackupOptions '{1}' parameter",
                this->TraceId,
                jetBackupOptions);

            return JET_errInvalidParameter;
        }

        InstanceRef ref(*this);

		if (!ref.IsValid)
		{
			return JET_errTermInProgress;
		}
        
		JET_ERR error = JET_errSuccess;

        bool truncate = dir.empty();
        
		std::wstring backupDir;

        if (truncate)
        {				
            ASSERT_IF(jetBackupOptions != JET_bitBackupAtomic, "{0} Empty backup path can only be used with jetBackupOption 'JET_bitBackupAtomic'.", this->TraceId);
            WriteInfo(TraceComponent, "{0} Empty backup path parameter provided. Truncating logs", this->TraceId);

			// The only documented way ESENT provides for truncation is to pass in NULL for the backupDir parameter in JetBackupInstance.
			// However this doesn't work when multiple threads truncate at the same time (JetBackupInstance internally creates a 
			// directory named 'new' under the process's executing folder, does a full backup there and deletes this folder. Now, multiple
			// threads are trying to create this 'new' directory, due to which full backup fails, and hence truncation fails.
			// As a workaround to truncate, we create a unique directory, do a full backup and then delete this directory
			// 
			// Longer term, we need to get a fix from ESE team or migrate to a newer version of ESENT (that has this fix).
			// Migration to a newer version is not straight forward since ESENT is not forward compatible. i.e. if we upgrade and downgrade,
			// then the newer database format cannot be accessed by the older version of ESENT.dll.
            //
			backupDir = GetTempTruncateDirectoryName(eseDirectory_);

			if (Directory::Exists(backupDir))
			{
				error = DeleteDirectoryForTruncation(backupDir, L"before");
				if (error != JET_errSuccess)
				{
					return error;
				}
			}
        }
		else
		{
			backupDir = dir;
		}
		
		error = CALL_ESE_NOTHROW(
			JetBackupInstance(
				instanceId_,
				backupDir.c_str(),
				jetBackupOptions,
				NULL));

		if (error != JET_errSuccess)
		{
			WriteInfo(
				TraceComponent,
				"{0} Backup instance failed. Instance Id: {1}, backup dir: {2}, backup options: {3}, error: {4}",
                this->TraceId,
				instanceId_,
				backupDir,
				jetBackupOptions,
				error);

			return error;
		}
		
		if (truncate)
		{			
			error = DeleteDirectoryForTruncation(backupDir, L"after");
		}

        return error;
    }

	/// <summary>
	/// Helper method that deletes the temporary directory before (if it exists) and after truncation
	/// </summary>
	/// <param name="directory">The directory to delete.</param>
	/// <param name="directory">A string just for additional context while tracing if the deletion is 'before' or 'after' truncation.</param>
	/// <returns>JET_errSuccess if successful. JET_errDeleteBackupFileFail otherwise.</returns>
	JET_ERR EseInstance::DeleteDirectoryForTruncation(std::wstring const & directory, std::wstring const & truncationStage)
	{
		JET_ERR error;

		ErrorCode errorCode = EseLocalStoreUtility::DeleteDirectoryWithRetries(directory, wformatString("{0}", instanceId_));

		if (errorCode.IsSuccess())
		{
			WriteInfo(
				TraceComponent,
				"{0} Successfully deleted temp directory '{1}' {2} truncating logs",
                this->TraceId,
				directory,
				truncationStage);

			error = JET_errSuccess;
		}
		else
		{
			// the closest matching JET error
			error = JET_errDeleteBackupFileFail;

			WriteWarning(
				TraceComponent,
				"{0} Failed to delete temp directory '{1}' {2} truncating logs. Error code: {3}. Corresponding JET error: {4}",
                this->TraceId,
				directory,
				truncationStage,
				errorCode,
				error);
		}

		return error;
	}

#if DBG
    bool EseInstance::IsSessionActive(EseSession const & session)
    {
        return (activeSessions_.find(session.SessionId) != activeSessions_.end());
    }

    void EseInstance::AssertSessionIsNotActive(EseSession const & session)
    {  
        AcquireReadLock acquire(thisLock_);

        ASSERT_IF(!aborting_ && this->IsSessionActive(session), "{0} Session should not be active.", this->TraceId);
    }
#endif // DBG

    JET_ERR EseInstance::AddActiveSession(EseSessionSPtr const & session)
    {  
        AcquireWriteLock acquire(thisLock_);

        if (!aborting_)
        {
            auto result = activeSessions_.insert(make_pair(session->SessionId, session));

            if (!result.second)
            {
                auto jetErr = JET_errSessionSharingViolation;

                TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} Duplicate ESE session id {1}: {2}", this->TraceId, session->SessionId, jetErr);

                return jetErr;
            }
            else
            {
                return JET_errSuccess;
            }
        }
        else
        {
            // This means we are about to call JetTerm, we need to abort the session
            // (i.e. rollback the transaction, if the session is in one)
            session->Abort();

            return JET_errTermInProgress;
        }
    }

    void EseInstance::RemoveActiveSession(EseSession const & session)
    {
        AcquireWriteLock acquire(thisLock_);

        activeSessions_.erase(session.SessionId);
    }

    JET_ERR EseInstance::SetSystemParameter(
        ULONG parameterId,
        std::wstring const & value)
    {
        AcquireReadLock acquire(thisLock_);

        return JetSetSystemParameter(
            &instanceId_,
            JET_sesidNil,
            parameterId,
            NULL,
            value.c_str());
    }

    wstring EseInstance::GetRestoreDirectoryName(std::wstring const & eseDirectory)
    {
        return wformatString("{0}_{1}", eseDirectory, *RestoreDirectorySuffix);
    }

    wstring EseInstance::GetLocalBackupDirectoryName(std::wstring const & eseDirectory)
    {
        return wformatString("{0}_{1}", eseDirectory, *LocalBackupDirectorySuffix);
    }

	wstring EseInstance::GetTempTruncateDirectoryName(std::wstring const & eseDirectory)
	{
		return wformatString("{0}_{1}", eseDirectory, *TempTruncateDirectoryPrefix);
	}

    JET_ERR EseInstance::GetSystemParameter(ULONG parameterId, __out size_t & result)
    {
        JET_API_PTR lParam = {0};

        AcquireReadLock acquire(thisLock_);

        auto jetErr = CALL_ESE_NOTHROW(
            JetGetSystemParameter(
                instanceId_,
                JET_sesidNil,
                parameterId,
                &lParam,
                NULL,
                0));

        if (JET_errSuccess == jetErr)
        {
            result = lParam;
        }

        return jetErr;
    }

    ErrorCode EseInstance::GetOpenFileSize(__out int64 & result)
    {
        result = openFileSize_;

        return ErrorCodeValue::Success;
    }

    JET_ERR EseInstance::SetSystemParameter(
        ULONG parameterId,
        JET_API_PTR lParam)
    {
        AcquireReadLock acquire(thisLock_);

        return JetSetSystemParameter(
            &instanceId_,
            JET_sesidNil,
            parameterId,
            lParam,
            NULL);
    }

    JET_API_PTR EseInstance::ConvertToDatabasePages(int cacheSizeInMB)
    {
        return (cacheSizeInMB * 1024 / (databasePageSize_ / 1024));
    }

    bool EseInstance::TryAddPendingCommit(EseCommitAsyncOperationSPtr const & commitOperation)
    {
        bool result = false;
        auto commitId = commitOperation->CommitId;

        AcquireWriteLock acquire(thisLock_);

        if (!CommitCompleted(commitId))
        {
            pendingCommits_[commitId] = commitOperation;
            WriteNoise(TraceComponent, "{0} There are {1} pending commits in AddPendingCommit.", this->TraceId, pendingCommits_.size());
            result = true;
        }

        return result;
    }

    bool EseInstance::CommitCompleted(__int64 commitId)
    {
        return commitId < nextBatchStartCommitId_;
    }

    void EseInstance::TryCompletePendingCommits(EseCommitsVector const & completedCommits, Common::ErrorCode const error)
    {
        perfCounters_->AvgCompletedCommitBatchSizeBase.Increment();
        perfCounters_->AvgCompletedCommitBatchSize.IncrementBy(completedCommits.size());

        Stopwatch stopwatch;

        for (size_t index = 0 ; index < completedCommits.size() ; ++index)
        {
            stopwatch.Restart();

            completedCommits[index]->ExternalComplete(completedCommits[index], error);

            stopwatch.Stop();

            perfCounters_->AvgCompletedCommitCallbackDurationBase.Increment();
            perfCounters_->AvgCompletedCommitCallbackDuration.IncrementBy(stopwatch.ElapsedMicroseconds);
        }
    }

    void EseInstance::OnFatalCommitError(Common::ErrorCode const error)
    {
        ASSERT_IFNOT(error.IsError(ErrorCodeValue::StoreFatalError), "{0} OnFatalSyncCommitError must be called only on StoreFatalError", this->TraceId);

        auto selfRoot = shared_from_this();
        Threadpool::Post([this, selfRoot, error]() { this->OnCommitComplete(MAXINT64, error); });
    }

    void EseInstance::OnCommitComplete(__int64 nextBatchStartCommitId, Common::ErrorCode const error)
    {
        bool setEvent = false;

        auto completedCommits = make_unique<EseCommitsVector>();
        completedCommits->reserve(200);
        {
            AcquireWriteLock acquire(thisLock_);
            if (!isHealthy_)
            {
                // If this is a commit callback from ESE after a sync-commit has failed due to a fatal error, we ignore it
                return;
            }

            if (error.IsError(ErrorCodeValue::StoreFatalError))
            {
                // Ese Commit callbacks do not have an error code. So this error is coming from one of our internal methods.
                isHealthy_ = false;
            }

            ASSERT_IFNOT(
                nextBatchStartCommitId >= nextBatchStartCommitId_, 
                "{0} Commit ids have to be non-decreasing: current={1} previous={2} error={3}", 
                this->TraceId,
                nextBatchStartCommitId,
                nextBatchStartCommitId_,
                error);

            if (nextBatchStartCommitId > nextBatchStartCommitId_)
            {
                nextBatchStartCommitId_ = nextBatchStartCommitId;

                auto it = pendingCommits_.begin();

                while (it != pendingCommits_.end() && it->first < nextBatchStartCommitId_)
                {
                    completedCommits->push_back(std::move(it->second));
                    ++it;
                }

                if (completedCommits->size() > 0)
                {
                    MoveUPtr<EseCommitsVector> completedCommitsMover(move(completedCommits));

                    pendingCommits_.erase(pendingCommits_.begin(), it);

                    if (pendingCommits_.empty() && 0 == instanceIdRef_ && aborting_)
                    {
                        setEvent = true;
                    }
                    WriteNoise(TraceComponent, "{0} There are {1} pending commits in OnCommitComplete.", this->TraceId, pendingCommits_.size());
  
                    //
                    // ESE holds semaphore while issuing this callback, hence use thread
                    // pool to do the completion of the async commits. No ESE calls should
                    // be made from the commit callback.
                    //
                    // Using the MoveUPtr allows us to transfer ownership of the async operations
                    // to TryCompletePendingCommits. It prevents shared ptr ref count to be
                    // decremented on the ESE callback thread, which can trigger destruction of
                    // the async operation and possible call to close the local store and JetTerm2
                    // to ESE.
                    //
                    auto selfRoot = shared_from_this();
                    Threadpool::Post([this, selfRoot, completedCommitsMover, error]() mutable 
                    { 
                        this->TryCompletePendingCommits(*(completedCommitsMover.TakeUPtr()), error); 
                    });
                    
                }
            }
        }

        if (setEvent)
        {
            allEseCallbacksCompleted_.Set();
        }
    }

    JET_ERR EseInstance::CommitCallback(
        JET_INSTANCE instance,
        JET_COMMIT_ID *pCommitIdSeen,
        JET_GRBIT grbit)
    {
        UNREFERENCED_PARAMETER(grbit);

        EseInstanceSPtr eseInstanceSPtr;
        {
            AcquireReadLock acquire(*EseInstance::sLock_);

            auto it = EseInstance::eseInstances_->find(instance);
            if (it != EseInstance::eseInstances_->end())
            {
                // Commit callback implies success
                eseInstanceSPtr = it->second;
            }
        }

        if (NULL != eseInstanceSPtr)
        {
            if (pCommitIdSeen->commitId == 0)
            {
                eseInstanceSPtr->WriteError(
                    TraceComponent, 
                    "{0} fatal error on commit: grbit=0x{1:x}",
                    eseInstanceSPtr->TraceId,
                    grbit);
            }

            eseInstanceSPtr->OnCommitComplete(pCommitIdSeen->commitId, ErrorCodeValue::Success);
        }

        return JET_errSuccess;
    }

    EseInstance::InstanceRef::InstanceRef(EseInstance & instance)
        : instance_(instance), locked_(false)
    {
        AcquireReadLock acquire(instance_.thisLock_);

        if (JET_instanceNil != instance.instanceId_ && !instance.aborting_ && instance.isHealthy_)
        {
            InterlockedIncrement(&instance_.instanceIdRef_);
            locked_ = true;
        }
    }

    EseInstance::InstanceRef::~InstanceRef()
    {
        bool setEvent = false;
        {
            AcquireReadLock acquire(instance_.thisLock_);

            if (locked_ && 0 == InterlockedDecrement(&instance_.instanceIdRef_) && instance_.aborting_)
            {
                setEvent = true;
            }
        }

        if (setEvent)
        {
            instance_.allEseCallsCompleted_.Set();
        }
    }
}
