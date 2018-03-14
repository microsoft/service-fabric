// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
	class CodePackageRuntimeInformation
	{
	public:
		CodePackageRuntimeInformation(std::wstring const & processPath, DWORD processId);

		__declspec(property(get=get_ExePath)) std::wstring const & ExePath;
		std::wstring const & get_ExePath() const;

		__declspec(property(get=get_ProcessId)) DWORD ProcessId;
		DWORD  get_ProcessId() const;

	private:
		std::wstring processPath_;
		DWORD processId_;
	};
}
