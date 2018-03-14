// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceMeasurement.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

ResourceMeasurement::ResourceMeasurement(uint64 memoryUsage, uint64 totalCpuTime, Common::DateTime const & timeRead)
    : memoryUsage_(memoryUsage),
      totalCpuTime_(totalCpuTime),
      timeRead_(timeRead)
{
}

void ResourceMeasurement::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceMeasurement { ");
    w.Write("MemoryUsage = {0}, ", memoryUsage_);
    w.Write("TotalCpuTime = {0},", totalCpuTime_);
    w.Write("TimeRead = {0}", timeRead_);
    w.Write("}");
}
