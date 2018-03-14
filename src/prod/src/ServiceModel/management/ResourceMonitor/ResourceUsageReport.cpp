// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceUsage.h"
#include "ResourceUsageReport.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

GlobalWString ResourceUsageReport::ResourceUsageReportAction = make_global<wstring>(L"ResourceUsageReportAction");

ResourceUsageReport::ResourceUsageReport(std::map<Reliability::PartitionId, ResourceUsage> && resourceUsageReports)
    : resourceUsageReports_(move(resourceUsageReports))
{
}

