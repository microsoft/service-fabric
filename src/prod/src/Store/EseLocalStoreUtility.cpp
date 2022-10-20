// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
	ErrorCode EseLocalStoreUtility::DeleteDirectoryWithRetries(std::wstring const & directory, std::wstring const & traceId)
	{
		WriteInfo(
			Constants::StoreSource,
			"{0} deleting all database files: {1}",
			traceId,
			directory);

		ErrorCode error = ErrorCodeValue::Success;
		if (Directory::Exists(directory))
		{
			int retryCount = StoreConfig::GetConfig().DeleteDatabaseRetryCount;
			bool success = false;

			while (!success && retryCount-- > 0)
			{
				error = Directory::Delete(directory, true);
				if (error.IsSuccess())
				{
					success = true;
				}
				else
				{
					WriteWarning(
						Constants::StoreSource,
						"{0} couldn't delete directory '{1}' because of error {2}: retry count = {3}",
						traceId,
						directory,
						error,
						retryCount);

					Sleep(StoreConfig::GetConfig().DeleteDatabaseRetryDelayMilliseconds);
				}
			}
		}

		return error;
	}
}
