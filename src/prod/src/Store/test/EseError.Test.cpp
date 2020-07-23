// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;

namespace Store
{
    class TestEseError
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestEseErrorSuite, TestEseError)

    BOOST_AUTO_TEST_CASE(ConvertErrors)
    {
        Config config;

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errSuccess).IsSuccess());
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnUniqueKey).IsSuccess());
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnSeekNotEqual).IsSuccess());
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnRecordFoundGreater).IsSuccess());
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnRecordFoundLess).IsSuccess());

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnBufferTruncated).IsError(ErrorCodeValue::StoreBufferTruncated));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_wrnSortOverflow).IsError(ErrorCodeValue::OutOfMemory));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errSPAvailExtCacheOutOfMemory).IsError(ErrorCodeValue::OutOfMemory));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errOutOfMemory).IsError(ErrorCodeValue::OutOfMemory));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errVersionStoreOutOfMemoryAndCleanupTimedOut).IsError(ErrorCodeValue::StoreTransactionTooLarge));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errVersionStoreOutOfMemory).IsError(ErrorCodeValue::StoreTransactionTooLarge));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errOutOfObjectIDs).IsError(ErrorCodeValue::StoreNeedsDefragment));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errOutOfLongValueIDs).IsError(ErrorCodeValue::StoreNeedsDefragment));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errOutOfDbtimeValues).IsError(ErrorCodeValue::StoreNeedsDefragment));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errOutOfSequentialIndexValues).IsError(ErrorCodeValue::StoreNeedsDefragment));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errSessionWriteConflict).IsError(ErrorCodeValue::StoreWriteConflict));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errWriteConflict).IsError(ErrorCodeValue::StoreWriteConflict));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errFileAccessDenied).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errSystemPathInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errLogFilePathInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errTempPathInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errInstanceNameInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errDatabaseInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errDatabaseIdInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errDatabaseSignInUse).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errDatabaseSharingViolation).IsError(ErrorCodeValue::StoreInUse));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errDatabaseLocked).IsError(ErrorCodeValue::StoreInUse));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errRecordNotFound).IsError(ErrorCodeValue::StoreRecordNotFound));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errKeyDuplicate).IsError(ErrorCodeValue::StoreRecordAlreadyExists));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errKeyTruncated).IsError(ErrorCodeValue::StoreKeyTooLarge));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errTermInProgress).IsError(ErrorCodeValue::ObjectClosed));


        StoreConfig::GetConfig().AssertOnFatalError = false;

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errLogWriteFail).IsError(ErrorCodeValue::StoreFatalError));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errLogDiskFull).IsError(ErrorCodeValue::StoreFatalError));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errInstanceUnavailable).IsError(ErrorCodeValue::StoreFatalError));
        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errInstanceUnavailableDueToFatalLogDiskFull).IsError(ErrorCodeValue::StoreFatalError));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errInvalidBackup).IsError(ErrorCodeValue::InvalidBackupSetting));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errMissingFullBackup).IsError(ErrorCodeValue::MissingFullBackup));

        VERIFY_IS_TRUE(EseError::GetErrorCode(JET_errBackupDirectoryNotEmpty).IsError(ErrorCodeValue::BackupDirectoryNotEmpty));
    }
	
	BOOST_AUTO_TEST_SUITE_END()
}
