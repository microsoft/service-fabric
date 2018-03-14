// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

CodePackageRuntimeInformation::CodePackageRuntimeInformation(std::wstring const & processPath, DWORD processId)
	: processPath_(processPath)
	, processId_(processId)
{
}

wstring const & CodePackageRuntimeInformation::get_ExePath() const
{
	return processPath_;
}

DWORD CodePackageRuntimeInformation::get_ProcessId() const
{
	return processId_;
}
