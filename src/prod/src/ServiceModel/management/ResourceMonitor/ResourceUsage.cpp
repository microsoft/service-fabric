// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceUsage.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

ResourceUsage::ResourceUsage(double cpuUsage, uint64 memoryUsage, double cpuUsageRaw, uint64 memoryUsageRaw)
    : cpuUsage_(cpuUsage),
      memoryUsage_(memoryUsage),
      cpuUsageRaw_(cpuUsageRaw),
      memoryUsageRaw_(memoryUsageRaw)
{
}

void ResourceUsage::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceUsage { ");
    w.Write("CpuUsage = {0}, ", cpuUsage_);
    w.Write("MemoryUsage = {0},", memoryUsage_);
    w.Write("CpuUsageRaw = {0},", cpuUsageRaw_);
    w.Write("MemoryUsageRaw = {0}", memoryUsageRaw_);
    w.Write("}");
}
