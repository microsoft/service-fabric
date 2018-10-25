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
            class ReconfigurationSlowEventData : public IEventData
            {
            public:                
                ReconfigurationSlowEventData(Common::Guid const & ftId, Federation::NodeInstance nodeInstance, std::wstring && reconfigurationPhase, std::wstring && traceDescription)
                    : ftId_(ftId), 
                    nodeInstance_(nodeInstance.getId().ToString()),
                    reconfigurationPhase_(reconfigurationPhase),
                    traceDescription_(traceDescription),
                    IEventData(TraceEventType::ReconfigurationSlow)
                {
                }

                __declspec (property(get = get_FtId)) const Common::Guid & FtId;
                const Common::Guid & get_FtId() const { return ftId_; }

                __declspec (property(get = get_NodeInstance)) const std::wstring & NodeInstance;
                const std::wstring & get_NodeInstance() const { return nodeInstance_; }

                __declspec (property(get = get_ReconfigurationPhase)) const std::wstring & ReconfigurationPhase;
                const std::wstring & get_ReconfigurationPhase() const { return reconfigurationPhase_; }

                __declspec (property(get = get_TraceDescription)) const std::wstring & TraceDescription;
                const std::wstring & get_TraceDescription() const { return traceDescription_; }

                std::shared_ptr<IEventData> Clone() const override
                {
                    return make_shared<ReconfigurationSlowEventData>(*this);
                }

            private:
                Common::Guid const ftId_;
                std::wstring nodeInstance_;
                std::wstring reconfigurationPhase_;
                std::wstring traceDescription_;

            };

            class ReconfigurationCompleteEventData : public IEventData
            {
            public:
                ReconfigurationCompleteEventData(Common::Guid const & ftId,
                    Federation::NodeInstance nodeInstance,
                    std::wstring const serviceType,
                    Reliability::Epoch ccEpoch,
                    ReconfigurationType::Enum type,
                    ReconfigurationResult::Enum result,
                    ReconfigurationPerformanceData perfData)
                    : ftId_(ftId),
                    nodeInstance_(nodeInstance.getId().ToString()),
                    serviceType_(serviceType),
                    ccEpoch_(ccEpoch),
                    type_(type),
                    result_(result),
                    perfData_(perfData),
                    IEventData(TraceEventType::ReconfigurationComplete)
                {
                }

                __declspec (property(get = get_FtId)) const Common::Guid & FtId;
                const Common::Guid & get_FtId() const { return ftId_; }

                __declspec (property(get = get_NodeInstance)) const std::wstring & NodeInstance;
                const std::wstring & get_NodeInstance() const { return nodeInstance_; }

                __declspec (property(get = get_ServiceType)) const std::wstring & ServiceType;
                const std::wstring & get_ServiceType() const { return serviceType_; }

                __declspec (property(get = get_CcEpoch)) const Reliability::Epoch & CcEpoch;
                const Reliability::Epoch & get_CcEpoch() const { return ccEpoch_; }

                __declspec (property(get = get_Type)) const ReconfigurationType::Enum & Type;
                const ReconfigurationType::Enum & get_Type() const { return type_; }

                __declspec (property(get = get_Result)) const ReconfigurationResult::Enum & Result;
                const ReconfigurationResult::Enum & get_Result() const { return result_; }

                __declspec (property(get = get_PerfData)) const ReconfigurationPerformanceData & PerfData;
                const ReconfigurationPerformanceData & get_PerfData() const { return perfData_; }

                std::shared_ptr<IEventData> Clone() const override
                {
                    return make_shared<ReconfigurationCompleteEventData>(*this);
                }

            private:
                Common::Guid const ftId_;
                std::wstring nodeInstance_;
                std::wstring const serviceType_;
                Reliability::Epoch const ccEpoch_;
                ReconfigurationType::Enum type_;
                ReconfigurationResult::Enum result_;
                ReconfigurationPerformanceData const perfData_;
            };

            class ReplicaStateChangeEventData : public IEventData
            {
            public:
                ReplicaStateChangeEventData(
                    std::wstring const & nodeInstance,
                    Common::Guid const & ftId,
                    int64 replicaId,
                    Epoch const & ccEpoch,
                    ReplicaLifeCycleState::Enum replicaState,
                    ReplicaRole::Enum replicaRole,
                    ActivityDescription const & activityDescription)
                    : ftId_(ftId),
                    nodeInstance_(nodeInstance),
                    replicaId_(replicaId),
                    ccEpoch_(ccEpoch),
                    replicaState_(replicaState),
                    replicaRole_(replicaRole),
                    reasonActivityDescription_(activityDescription),
                    IEventData(TraceEventType::ReplicaStateChange)
                {
                }

                __declspec (property(get = get_NodeInstance)) const std::wstring & NodeInstance;
                const std::wstring & get_NodeInstance() const { return nodeInstance_; }

                __declspec (property(get = get_FtId)) const Common::Guid & FtId;
                const Common::Guid & get_FtId() const { return ftId_; }

                __declspec (property(get = get_ReplicaId)) int64 ReplicaId;
                int64 get_ReplicaId() const { return replicaId_; }

                __declspec (property(get = get_CcEpoch)) const Reliability::Epoch & CurrentConfigurationEpoch;
                const Reliability::Epoch & get_CcEpoch() const { return ccEpoch_; }

                __declspec (property(get = get_ReplicaState)) const ReplicaLifeCycleState::Enum & ReplicaState;
                const ReplicaLifeCycleState::Enum & get_ReplicaState() const { return replicaState_; }

                __declspec (property(get = get_ReplicaRole)) const ReplicaRole::Enum & ReplicaRole;
                const ReplicaRole::Enum & get_ReplicaRole() const { return replicaRole_; }

                __declspec (property(get = get_Reason)) const Common::ActivityDescription & Reason;
                const Common::ActivityDescription & get_Reason() const { return reasonActivityDescription_; }

                std::shared_ptr<IEventData> Clone() const override
                {
                    return make_shared<ReplicaStateChangeEventData>(*this);
                }

            private:
                std::wstring nodeInstance_;
                Common::Guid ftId_;
                int64 const replicaId_;
                Reliability::Epoch ccEpoch_;
                ReplicaLifeCycleState::Enum replicaState_;
                ReplicaRole::Enum replicaRole_;
                Common::ActivityDescription const reasonActivityDescription_;
            };
        }
    }
}
