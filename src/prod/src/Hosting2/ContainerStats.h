// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CpuUsage : public Common::IFabricJsonSerializable
    {
    public:
        CpuUsage() = default;
        ~CpuUsage() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(CpuUsage::TotalUsageParameter, TotalUsage_, (TotalUsage_ != 0))
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:

        uint64 TotalUsage_;

        static Common::WStringLiteral const TotalUsageParameter;
    };

    class CpuStats : public Common::IFabricJsonSerializable
    {
    public:
        CpuStats() = default;
        ~CpuStats() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(CpuStats::CpuUsageParameter, CpuUsage_)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        CpuUsage CpuUsage_;

        static Common::WStringLiteral const CpuUsageParameter;
    };

    class MemoryStats : public Common::IFabricJsonSerializable
    {
    public:
        MemoryStats() = default;
        ~MemoryStats() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(MemoryStats::MemoryUsageParameter, MemoryUsage_, (MemoryUsage_ != 0))
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        uint64 MemoryUsage_;

        static Common::WStringLiteral const MemoryUsageParameter;

    };

    class ContainerStatsResponse : public Common::IFabricJsonSerializable
    {
    public:
        ContainerStatsResponse() = default;
        ~ContainerStatsResponse() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerStatsResponse::ReadParameter, Read_)
            SERIALIZABLE_PROPERTY(ContainerStatsResponse::CpuStatsParameter, CpuStats_)
            SERIALIZABLE_PROPERTY(ContainerStatsResponse::MemoryStatsParameter, MemoryStats_)
        END_JSON_SERIALIZABLE_PROPERTIES()
    public:
        CpuStats CpuStats_;
        MemoryStats MemoryStats_;
        Common::DateTime Read_;

        static Common::WStringLiteral const CpuStatsParameter;
        static Common::WStringLiteral const MemoryStatsParameter;
        static Common::WStringLiteral const ReadParameter;

    };
}
