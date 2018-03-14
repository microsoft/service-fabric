// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class FailoverUnitDescription : public Serialization::FabricSerializable
    {

    public:

        FailoverUnitDescription();

        FailoverUnitDescription(
            Reliability::FailoverUnitId failoverUnitId,
            Reliability::ConsistencyUnitDescription const& consistencyUnitDescription,
            Reliability::Epoch ccEpoch = Epoch(), 
            Reliability::Epoch pcEpoch = Epoch());

        __declspec (property(get=get_FailoverUnitId)) Reliability::FailoverUnitId const& FailoverUnitId;
        Reliability::FailoverUnitId const& get_FailoverUnitId() const { return failoverUnitId_; }

        __declspec (property(get=get_isForFMReplica)) bool IsForFMReplica;
        bool get_isForFMReplica() const { return failoverUnitId_.IsFM; }

        __declspec (property(get=get_ConsistencyUnitDescription)) Reliability::ConsistencyUnitDescription const& ConsistencyUnitDescription;
        Reliability::ConsistencyUnitDescription const& get_ConsistencyUnitDescription() const { return consistencyUnitDescription_; }

        __declspec (property(get=get_CurrentConfigurationEpoch, put=set_CurrentConfigurationEpoch)) Reliability::Epoch CurrentConfigurationEpoch;
        Reliability::Epoch const & get_CurrentConfigurationEpoch() const { return ccEpoch_; }
        void set_CurrentConfigurationEpoch(Reliability::Epoch value) { ccEpoch_ = value; }

        __declspec (property(get=get_PreviousConfigurationEpoch, put=set_PreviousConfigurationEpoch)) Reliability::Epoch PreviousConfigurationEpoch;
        Reliability::Epoch const & get_PreviousConfigurationEpoch() const { return pcEpoch_; }
        void set_PreviousConfigurationEpoch(Reliability::Epoch value) { pcEpoch_ = value; }

        __declspec (property(get=get_TargetReplicaSetSize, put=set_TargetReplicaSetSize)) int TargetReplicaSetSize;
        int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }
        void set_TargetReplicaSetSize(int value) { targetReplicaSetSize_  = value; }

        __declspec (property(get=get_MinReplicaSetSize, put=set_MinReplicaSetSize)) int MinReplicaSetSize;
        int get_MinReplicaSetSize() const { return minReplicaSetSize_; }
        void set_MinReplicaSetSize(int value) { minReplicaSetSize_ = value; }

        __declspec(property(get = get_IsPrimaryChangeBetweenPCAndCC)) bool IsPrimaryChangeBetweenPCAndCC;
        bool get_IsPrimaryChangeBetweenPCAndCC() const 
        { 
            ASSERT_IF(pcEpoch_.IsInvalid(), "Cannot call this if pc epoch is not valid");
            return pcEpoch_.ToPrimaryEpoch() != ccEpoch_.ToPrimaryEpoch(); 
        }

        __declspec(property(get = get_IsDataLossBetweenPCAndCC)) bool IsDataLossBetweenPCAndCC;
        bool get_IsDataLossBetweenPCAndCC() const
        {
            ASSERT_IF(pcEpoch_.IsInvalid(), "Cannot call this if pc epoch is not valid");
            return pcEpoch_.DataLossVersion != ccEpoch_.DataLossVersion;            
        }

        FailoverUnitDescription & operator = (FailoverUnitDescription const & other);

        bool IsReservedCUID() const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_FIELDS_06(failoverUnitId_, consistencyUnitDescription_, ccEpoch_, pcEpoch_, targetReplicaSetSize_, minReplicaSetSize_);

    private:

        Reliability::FailoverUnitId failoverUnitId_;
        Reliability::ConsistencyUnitDescription consistencyUnitDescription_;
        Reliability::Epoch ccEpoch_;
        Reliability::Epoch pcEpoch_;
        int targetReplicaSetSize_;
        int minReplicaSetSize_;
    };
}
