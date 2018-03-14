// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceUsage : public Serialization::FabricSerializable
        {
        public:

            ResourceUsage() = default;
            ResourceUsage(double cpuUsage, uint64 memoryUsage, double cpuUsageRaw, uint64 memoryUsageRaw);

            __declspec(property(get=get_CpuLoad,put=put_CpuLoad)) double CpuUsage;
            double get_CpuLoad() const { return cpuUsage_; }
            void put_CpuLoad(double value) { cpuUsage_ = value; }

            __declspec(property(get = get_MemoryUsage,put=put_MemoryUsage)) uint64 MemoryUsage;
            uint64 get_MemoryUsage() const { return memoryUsage_; }
            void put_MemoryUsage(uint64 value) { memoryUsage_ = value; }

            __declspec(property(get = get_CpuLoadRaw, put = put_CpuLoadRaw)) double CpuUsageRaw;
            double get_CpuLoadRaw() const { return cpuUsageRaw_; }
            void put_CpuLoadRaw(double value) { cpuUsageRaw_ = value; }

            __declspec(property(get = get_MemoryUsageRaw, put = put_MemoryUsageRaw)) uint64 MemoryUsageRaw;
            uint64 get_MemoryUsageRaw() const { return memoryUsageRaw_; }
            void put_MemoryUsageRaw(uint64 value) { memoryUsageRaw_ = value; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(cpuUsage_, memoryUsage_, cpuUsageRaw_, memoryUsageRaw_);

        private:
            //we send out adjusted cpu usage to PLB but trace out raw usage as well
            double cpuUsage_;
            uint64 memoryUsage_;
            double cpuUsageRaw_;
            uint64 memoryUsageRaw_;
        };
    }
}
