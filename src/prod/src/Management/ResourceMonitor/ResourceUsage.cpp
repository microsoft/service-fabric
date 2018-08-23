// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;


ResourceUsage::ResourceUsage()
{
    memoryUsage_ = 0;
    totalCpuTime_ = 0;
    cpuRate_ = 0.0;
    lastRead_ = DateTime::get_Now();
    timeMeasured_ = 0;
}

ResourceUsage::~ResourceUsage()
{

}

bool ResourceUsage::ShouldCheckUsage(DateTime now)
{
    return timeMeasured_ == 0 || lastRead_ + ResourceMonitorServiceConfig::GetConfig().HostResourceCheck <= now;
}

void ResourceUsage::Update(Management::ResourceMonitor::ResourceMeasurement const & measurement)
{
    //we are interested in MB - we get the memory in bytes
    uint64 memoryUsageInMB = measurement.MemoryUsage / (1024 * 1024);
    //if this is not our first measure we can calculate the cpu rate
    if (timeMeasured_ > 0)
    {
        TimeSpan timeSpan = (measurement.TimeRead - lastRead_);
        int64 timeSpanInTicks = timeSpan.get_Ticks();
        double cpuRatePerTimeSpanTick = (double)(CpuRateTicksPerSecond / TimeSpan::TicksPerSecond);
        cpuRateCurrent_ = (measurement.TotalCpuTime - totalCpuTime_) / (cpuRatePerTimeSpanTick * timeSpanInTicks);

        double lambdaMemory = ResourceMonitorServiceConfig::GetConfig().MemorySmoothingFactor;
        double lambdaCpu = ResourceMonitorServiceConfig::GetConfig().CpuSmoothingFactor;

        //if we had previous measures we can do smoothing else just take the current values
        if (timeMeasured_ > 1)
        {
            cpuRate_ = cpuRate_ * (1 - lambdaCpu) + cpuRateCurrent_ * lambdaCpu;
        }
        else
        {
            cpuRate_ = cpuRateCurrent_;
        }
        memoryUsage_ = static_cast<uint64>(static_cast<double>(memoryUsage_) * (1 - lambdaMemory) + static_cast<double>(memoryUsageInMB) * lambdaMemory);
    }
    //we cannot really calculate the cpu rate given that this is the first time we are measuring so just set the memory for now
    else
    {
        memoryUsage_ = memoryUsageInMB;
    }

    lastRead_ = measurement.TimeRead;
    totalCpuTime_ = measurement.TotalCpuTime;
    memoryUsageCurrent_ = memoryUsageInMB;
    ++timeMeasured_;

}

void ResourceUsage::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceUsage { ");
    w.Write("MemoryUsage = {0}, ", memoryUsage_);
    w.Write("TotalCpuTime = {0},", totalCpuTime_);
    w.Write("CpuRate = {0},", cpuRate_);
    w.Write("LastRead = {0},", lastRead_);
    w.Write("MemoryUsageCurrent = {0}, ", memoryUsageCurrent_);
    w.Write("CpuRateCurrent = {0},", cpuRateCurrent_);
    w.Write("TimeMeasured = {0}", timeMeasured_);
    w.Write("}");
}
