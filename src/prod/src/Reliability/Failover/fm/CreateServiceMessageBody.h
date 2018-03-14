// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class CreateServiceMessageBody : public Serialization::FabricSerializable
    {
    public:
        CreateServiceMessageBody()
        {
        }

        CreateServiceMessageBody(
            ServiceDescription const& serviceDescription,
            std::vector<ConsistencyUnitDescription> const& consistencyUnitDescriptions)
            : serviceDescription_(serviceDescription), consistencyUnitDescriptions_(consistencyUnitDescriptions)
        {
        }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec (property(get=get_ConsistencyUnitDescriptions)) std::vector<ConsistencyUnitDescription> const& ConsistencyUnitDescriptions;
        std::vector<ConsistencyUnitDescription> const& get_ConsistencyUnitDescriptions() const { return consistencyUnitDescriptions_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w << serviceDescription_;

            w << "[ ";
            for (size_t i = 0; i < consistencyUnitDescriptions_.size(); i++)
            {
                w << consistencyUnitDescriptions_[i] << ' ';
            }
            w << "] ";
        }

        FABRIC_FIELDS_02(serviceDescription_, consistencyUnitDescriptions_);

    private:
        Reliability::ServiceDescription serviceDescription_;
        std::vector<ConsistencyUnitDescription> consistencyUnitDescriptions_;
    };
}
