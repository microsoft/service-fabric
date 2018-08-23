// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ContainerStats.h"

using namespace Hosting2;


Common::WStringLiteral const CpuUsage::TotalUsageParameter(L"total_usage");

Common::WStringLiteral const CpuStats::CpuUsageParameter(L"cpu_usage");

#if defined(PLATFORM_UNIX)
Common::WStringLiteral const MemoryStats::MemoryUsageParameter(L"usage");
#else
Common::WStringLiteral const MemoryStats::MemoryUsageParameter(L"privateworkingset");
#endif

Common::WStringLiteral const ContainerStatsResponse::MemoryStatsParameter(L"memory_stats");
Common::WStringLiteral const ContainerStatsResponse::CpuStatsParameter(L"cpu_stats");
Common::WStringLiteral const ContainerStatsResponse::ReadParameter(L"read");
