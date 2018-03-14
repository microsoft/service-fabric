// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

ResourceMonitor::TelemetryEvent::TelemetryEvent(std::wstring const & partitionId, std::vector<Metric> const & eventMetrics):
    partitionId_(partitionId),
    eventMetrics_(eventMetrics)
{
}
