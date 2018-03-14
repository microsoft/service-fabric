// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpdateServiceRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        NodeUpdateServiceRequestMessageBody()
        {
        }

        NodeUpdateServiceRequestMessageBody(ServiceDescription const& serviceDescription)
            : serviceDescription_(serviceDescription)
        {
        }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec(property(get = get_Owner)) Reliability::FailoverManagerId Owner;
        Reliability::FailoverManagerId get_Owner() const
        {
            return serviceDescription_.Name == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName ? *FailoverManagerId::Fmm : *FailoverManagerId::Fm;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w << serviceDescription_;
        }

        FABRIC_FIELDS_01(serviceDescription_);

    private:
        Reliability::ServiceDescription serviceDescription_;
    };
}
