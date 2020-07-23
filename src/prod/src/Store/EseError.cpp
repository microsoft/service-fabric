// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#if !defined(JET_errBackupAbortByServer)
#define JET_errBackupAbortByServer -801
#endif

namespace Store
{
    using namespace Common;

    ErrorCode EseError::GetErrorCode(JET_ERR jetError)
    {
        if (jetError == JET_errSuccess)
        {
            return ErrorCode::Success();
        }

        static_assert(
            JET_wrnSeekNotEqual == JET_wrnRecordFoundGreater,
            "If this fails, we need separate cases for these in the switch below.");
        static_assert(
            JET_wrnSeekNotEqual == JET_wrnRecordFoundLess,
            "If this fails, we need separate cases for these in the switch below.");

        ErrorCodeValue::Enum errValue = ErrorCodeValue::StoreUnexpectedError;

        switch (jetError)
        {
            // These can occur normally when you do a seek.
        case JET_wrnUniqueKey:
        case JET_wrnSeekNotEqual:
            // case JET_wrnRecordFoundGreater:  // same as JET_wrnSeekNotEqual
            // case JET_wrnRecordFoundLess:     // same as JET_wrnSeekNotEqual
            return ErrorCode::Success();

        case JET_wrnBufferTruncated:
            errValue = ErrorCodeValue::StoreBufferTruncated;
            break;

        case JET_wrnSortOverflow:
        case JET_errSPAvailExtCacheOutOfMemory:
        case JET_errOutOfMemory:
            errValue = ErrorCodeValue::OutOfMemory;
            break;

        case JET_errVersionStoreOutOfMemoryAndCleanupTimedOut:
        case JET_errVersionStoreOutOfMemory:
            errValue = ErrorCodeValue::StoreTransactionTooLarge;
            break;

        case JET_errOutOfObjectIDs:
        case JET_errOutOfLongValueIDs:
        case JET_errOutOfDbtimeValues:
        case JET_errOutOfSequentialIndexValues:
            errValue = ErrorCodeValue::StoreNeedsDefragment;
            break;

        case JET_errSessionWriteConflict:
        case JET_errWriteConflict:
            errValue = ErrorCodeValue::StoreWriteConflict;
            break;

        case JET_errFileAccessDenied:
        case JET_errSystemPathInUse:
        case JET_errLogFilePathInUse:
        case JET_errTempPathInUse:
        case JET_errInstanceNameInUse:
        case JET_errDatabaseInUse:
        case JET_errDatabaseIdInUse:
        case JET_errDatabaseSignInUse:
        case JET_errDatabaseSharingViolation:
        case JET_errDatabaseLocked:
        case JET_errTooManyActiveUsers:
            errValue = ErrorCodeValue::StoreInUse;
            break;

        case JET_errRecordNotFound:
            errValue = ErrorCodeValue::StoreRecordNotFound;
            break;

        case JET_errKeyDuplicate:
            errValue = ErrorCodeValue::StoreRecordAlreadyExists;
            break;

        case JET_errKeyTruncated:
            errValue = ErrorCodeValue::StoreKeyTooLarge;
            break;

        case JET_errBackupAbortByServer: // Pending backups are aborted during EseInstance::OnClose()
        case JET_errTermInProgress:
            errValue = ErrorCodeValue::ObjectClosed;
            break;

        // backup errors
        //
        case JET_errInvalidBackup:
            errValue = ErrorCodeValue::InvalidBackupSetting;
            break;

        case JET_errMissingFullBackup:
            errValue = ErrorCodeValue::MissingFullBackup;
            break;

        case JET_errBackupDirectoryNotEmpty:
            errValue = ErrorCodeValue::BackupDirectoryNotEmpty;
            break;

        case JET_errDeleteBackupFileFail:
            errValue = ErrorCodeValue::DeleteBackupFileFailed;
            break;

        case JET_errBackupInProgress:
            errValue = ErrorCodeValue::BackupInProgress;
            break;

        case JET_errSessionSharingViolation:
        case JET_errSessionContextAlreadySet:
            errValue = ErrorCodeValue::MultithreadedTransactionsNotAllowed;
            break;

        // Hardware corruption most likely
        case JET_errDiskIO:
        case JET_errLogReadVerifyFailure:
        case JET_errReadVerifyFailure:
        case JET_errLogFileCorrupt:
        case JET_errCheckpointCorrupt:
        case JET_errDatabaseCorrupted:
        // File mis-management or filesystem errors most likely
        case JET_errAttachedDatabaseMismatch:
        case JET_errDatabaseLogSetMismatch:
        case JET_errRequiredLogFilesMissing:
        case JET_errMissingLogFile:
        case JET_errInvalidLogSequence:
        case JET_errDatabaseInconsistent:
        case JET_errFileNotFound:
        case JET_errInvalidPath:
        case JET_errPatchFileMissing:
        case JET_errConsistentTimeMismatch:
        case JET_errBadPatchPage:
        case JET_errDirtyShutdown:
        case JET_errBadLogVersion:
            errValue = ErrorCodeValue::DatabaseFilesCorrupted;
            break;

        // Unrecoverable errors that leave the ESE instance
        // in a permanently faulted state until it's restarted
        //
        case JET_errLogWriteFail:
        case JET_errLogDiskFull:
        case JET_errInstanceUnavailable:
        case JET_errInstanceUnavailableDueToFatalLogDiskFull:
            errValue = ErrorCodeValue::StoreFatalError;
            break;

        default:
            errValue = ErrorCodeValue::StoreUnexpectedError;
            break;
        }

        return GetErrorCodeWithMessage(jetError, errValue);
    }

    ErrorCode EseError::GetErrorCodeWithMessage(JET_ERR jetError, Common::ErrorCodeValue::Enum errValue)
    {
        auto msg = wformatString(GET_STORE_RC( ESE_Error ), jetError);

        if (errValue == ErrorCodeValue::StoreUnexpectedError)
        {
            EseLocalStore::WriteWarning("Failure", "{0}", msg);
        }

        return ErrorCode(errValue, move(msg));
    }
}
