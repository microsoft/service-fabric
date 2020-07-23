// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
	/// <summary>
	/// Helper class for common ESE local store functionality.
	/// </summary>
	class EseLocalStoreUtility : public Common::TextTraceComponent<Common::TraceTaskCodes::EseStore>
	{		
		DENY_COPY(EseLocalStoreUtility);

	private:
		EseLocalStoreUtility();

	public:
		static Common::ErrorCode DeleteDirectoryWithRetries(std::wstring const & directory, std::wstring const & traceId);
	};
}
