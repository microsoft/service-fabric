// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Diagnostics.IEventData.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class ResourceUsageReportEventData : public IEventData
            {
            public:
                ResourceUsageReportEventData(Common::Guid const & ftId,
                    Federation::NodeInstance nodeInstance,
                    ReplicaRole::Enum replicaRole,
                    int64 replicaId,
                    double cpuUsage,
                    int64 memoryUsage,
                    double cpuUsageRaw,
                    int64 memoryUsageRaw)
                    : ftId_(ftId),
                    nodeInstance_(nodeInstance.getId().ToString()),
                    replicaRole_(replicaRole),
                    replicaId_(replicaId),
                    cpuUsage_(cpuUsage),
                    memoryUsage_(memoryUsage),
                    cpuUsageRaw_(cpuUsageRaw),
                    memoryUsageRaw_(memoryUsageRaw),
                    IEventData(TraceEventType::ResourceUsageReport)
                {
                }

                __declspec (property(get = get_FtId)) const Common::Guid & FtId;
                const Common::Guid & get_FtId() const { return ftId_; }

                __declspec (property(get = get_NodeInstance)) const std::wstring & NodeInstance;
                const std::wstring & get_NodeInstance() const { return nodeInstance_; }

                __declspec (property(get = get_ReplicaRole)) ReplicaRole::Enum ReplicaRole;
                ReplicaRole::Enum get_ReplicaRole() const { return replicaRole_; }

                __declspec (property(get = get_ReplicaId)) int64 ReplicaId;
                int64 get_ReplicaId() const { return replicaId_; }

                __declspec (property(get = get_CpuUsage)) double CpuUsage;
                double get_CpuUsage() const { return cpuUsage_; }

                __declspec (property(get = get_MemoryUsage)) int64 MemoryUsage;
                int64 get_MemoryUsage() const { return memoryUsage_; }

                __declspec (property(get = get_CpuUsageRaw)) double CpuUsageRaw;
                double get_CpuUsageRaw() const { return cpuUsageRaw_; }

                __declspec (property(get = get_MemoryUsageRaw)) int64 MemoryUsageRaw;
                int64 get_MemoryUsageRaw() const { return memoryUsageRaw_; }

                std::shared_ptr<IEventData> Clone() const override
                {
                    return make_shared<ResourceUsageReportEventData>(*this);
                }

            private:
                Common::Guid const ftId_;
                std::wstring nodeInstance_;
                ReplicaRole::Enum replicaRole_;
                int64 replicaId_;
                double cpuUsage_;
                int64 memoryUsage_;
                double cpuUsageRaw_;
                int64 memoryUsageRaw_;

            };
        }
    }
}
