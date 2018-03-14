// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReplicaDeactivationInfo : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(ReplicaDeactivationInfo)
        DEFAULT_COPY_ASSIGNMENT(ReplicaDeactivationInfo)
    
    public:
        static const ReplicaDeactivationInfo Dropped;

        ReplicaDeactivationInfo();

        ReplicaDeactivationInfo(
            Reliability::Epoch const & deactivationEpoch,
            int64 catchupLSN);

        ReplicaDeactivationInfo(ReplicaDeactivationInfo && other);
        ReplicaDeactivationInfo& operator=(ReplicaDeactivationInfo && other);

        __declspec(property(get = get_IsValid)) bool IsValid;
        bool get_IsValid() const { return !IsInvalid; }     

        __declspec(property(get = get_IsInvalid)) bool IsInvalid;
        bool get_IsInvalid() const { return deactivationEpoch_.IsInvalid(); }     

        __declspec(property(get = get_IsDropped)) bool IsDropped;
        bool get_IsDropped() const { return *this == ReplicaDeactivationInfo::Dropped; }

        __declspec(property(get = get_DeactivationEpoch)) Reliability::Epoch const & DeactivationEpoch;
        Reliability::Epoch const & get_DeactivationEpoch() const { return deactivationEpoch_; } 

        __declspec(property(get = get_CatchupLSN)) int64 CatchupLSN;
        int64 get_CatchupLSN() const { return catchupLSN_; } 

        bool IsDeactivatedIn(Reliability::Epoch const & other) const;

        FABRIC_FIELDS_02(deactivationEpoch_, catchupLSN_);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        void Clear();

        bool operator<(ReplicaDeactivationInfo const & other) const;
        bool operator<=(ReplicaDeactivationInfo const & other) const;
        bool operator>(ReplicaDeactivationInfo const & other) const;
        bool operator>=(ReplicaDeactivationInfo const & other) const;
        bool operator==(ReplicaDeactivationInfo const & other) const;        
        bool operator!=(ReplicaDeactivationInfo const & other) const;
        
        static ReplicaDeactivationInfo CreateDroppedDeactivationInfo();
    private:
        void AssertCatchupLSNMustMatch(int64 otherCatchupLSN) const;

        Reliability::Epoch deactivationEpoch_;
        int64 catchupLSN_;
    };
}

