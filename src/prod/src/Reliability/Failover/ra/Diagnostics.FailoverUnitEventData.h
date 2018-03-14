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
        }
    }
}
