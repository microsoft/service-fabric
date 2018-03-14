// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceMeasurement : public Serialization::FabricSerializable
        {
        public:

            ResourceMeasurement() = default;
            ResourceMeasurement(uint64 memoryUsage, uint64 totalCpuTime, Common::DateTime const & timeRead);

            __declspec(property(get = get_MemoryUsage,put=put_MemoryUsage)) uint64 MemoryUsage;
            uint64 get_MemoryUsage() const { return memoryUsage_; }
            void put_MemoryUsage(uint64 value) { memoryUsage_ = value; }

            __declspec(property(get=get_CpuLoad,put=put_CpuLoad)) uint64 TotalCpuTime;
            uint64 get_CpuLoad() const { return totalCpuTime_; }
            void put_CpuLoad(uint64 value) { totalCpuTime_ = value; }

            __declspec(property(get = get_Time,put=put_Time)) Common::DateTime TimeRead;
            Common::DateTime get_Time() const { return timeRead_; }
            void put_Time(Common::DateTime value) { timeRead_ = value; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(memoryUsage_, totalCpuTime_, timeRead_);

        private:
            uint64 memoryUsage_;
            uint64 totalCpuTime_;
            Common::DateTime timeRead_;
        };
    }
}
