// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReportFaultMessageBody : public Serialization::FabricSerializable
        {
        public:

            ReportFaultMessageBody()
            {
            }

            ReportFaultMessageBody(
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & replicaDesc,
                FaultType::Enum faultType,
                Common::ActivityDescription const & activityDescription) :
                fudesc_(fudesc),
                replicaDesc_(replicaDesc),
                faultType_(faultType),
                activityDescription_(activityDescription)
            {
            }

            __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

            __declspec (property(get=get_LocalReplicaDescription)) Reliability::ReplicaDescription const & LocalReplicaDescription;
            Reliability::ReplicaDescription const & get_LocalReplicaDescription() const { return replicaDesc_; }

            __declspec(property(get=get_FaultType)) FaultType::Enum FaultType;
            FaultType::Enum get_FaultType() const { return faultType_; }

            __declspec(property(get = get_ActivityDescription)) Common::ActivityDescription const & ActivityDescription;
            Common::ActivityDescription const & get_ActivityDescription() const { return activityDescription_; }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
            {
                writer.Write("{0} {1} {2} {3}", fudesc_, replicaDesc_, faultType_, activityDescription_);
            }

            void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_04(fudesc_, replicaDesc_, faultType_, activityDescription_);

        private:
            Reliability::FailoverUnitDescription fudesc_;
            Reliability::ReplicaDescription replicaDesc_;
            Reliability::FaultType::Enum faultType_;
            Common::ActivityDescription activityDescription_;
        };
    }
}
