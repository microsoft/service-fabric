// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

//
// Do not change application shared log location or name to preserve backwards compability with old V2 managed stack
//
std::wstring const KtlLogger::Constants::DefaultApplicationSharedLogIdString(L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62");
Common::Guid const KtlLogger::Constants::DefaultApplicationSharedLogId(DefaultApplicationSharedLogIdString);
std::wstring const KtlLogger::Constants::DefaultApplicationSharedLogName(L"replicatorshared.log");
std::wstring const KtlLogger::Constants::DefaultApplicationSharedLogSubdirectory(L"ReplicatorLog");

std::wstring const KtlLogger::Constants::DefaultSystemServicesSharedLogIdString(L"15B07384-83C5-411D-9942-1655520C77C6");
Common::Guid const KtlLogger::Constants::DefaultSystemServicesSharedLogId(DefaultSystemServicesSharedLogIdString);
std::wstring const KtlLogger::Constants::DefaultSystemServicesSharedLogName(L"sysshared.log");
std::wstring const KtlLogger::Constants::DefaultSystemServicesSharedLogSubdirectory(L"SysLog");
std::wstring const KtlLogger::Constants::NullGuidString(L"00000000-0000-0000-0000-000000000000");
