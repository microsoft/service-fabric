// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReportFaultRequestMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        ReportFaultRequestMessageBody();

        ReportFaultRequestMessageBody(
            std::wstring nodeName,
            FaultType::Enum faultType,
            int64 replicaId,
            Common::Guid partitionId,
            bool isForce);

        ReportFaultRequestMessageBody(ReportFaultRequestMessageBody && other);

        ReportFaultRequestMessageBody & operator=(ReportFaultRequestMessageBody && other);

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_FaultType)) FaultType::Enum FaultType;
        FaultType::Enum get_FaultType() const { return faultType_; }

        __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
        int64 get_ReplicaId() const { return replicaId_; }

        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return partitionId_; }

        __declspec(property(get = get_IsForce)) bool IsForce;
        bool get_IsForce() const { return isForce_; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        void FillEventData(Common::TraceEventContext & context) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_05(nodeName_, faultType_, replicaId_, partitionId_, isForce_);

    private:
        std::wstring nodeName_;
        FaultType::Enum faultType_;
        int64 replicaId_;
        Common::Guid partitionId_;
        bool isForce_;
    };
}
