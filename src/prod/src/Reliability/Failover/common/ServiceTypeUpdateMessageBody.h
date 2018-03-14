// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTypeUpdateMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeUpdateMessageBody()
        {
        }

        ServiceTypeUpdateMessageBody(std::vector<ServiceModel::ServiceTypeIdentifier> && serviceTypes, uint64 sequenceNumber)
            : serviceTypes_(std::move(serviceTypes)), sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_ServiceTypes)) std::vector<ServiceModel::ServiceTypeIdentifier> const& ServiceTypes;
        std::vector<ServiceModel::ServiceTypeIdentifier> const& get_ServiceTypes() const { return serviceTypes_; }

        __declspec (property(get=get_SequenceNumber)) uint64 SequenceNumber;
        uint64 get_SequenceNumber() const { return sequenceNumber_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.WriteLine("SeqNum: {0}", sequenceNumber_);

            for (ServiceModel::ServiceTypeIdentifier const& serviceType : serviceTypes_)
            {
                w.WriteLine("{0} ", serviceType);
            }
        }

        FABRIC_FIELDS_02(serviceTypes_, sequenceNumber_);

    private:

        std::vector<ServiceModel::ServiceTypeIdentifier> serviceTypes_;
        uint64 sequenceNumber_;
    };
}
