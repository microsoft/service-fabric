// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceDescriptionReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        ServiceDescriptionReplyMessageBody()
            : serviceDescription_(),
            error_(),
            consistencyUnitDescriptions_()
        {
        }

        ServiceDescriptionReplyMessageBody(
            Reliability::ServiceDescription const& serviceDescription,
            Common::ErrorCodeValue::Enum error)
            : serviceDescription_(serviceDescription),
            error_(error),
            consistencyUnitDescriptions_()
        {
        }

        ServiceDescriptionReplyMessageBody(
            Reliability::ServiceDescription const& serviceDescription,
            Common::ErrorCodeValue::Enum error,
            std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions)
            : serviceDescription_(serviceDescription),
            error_(error),
            consistencyUnitDescriptions_(std::move(consistencyUnitDescriptions))
        {
        }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec (property(get = get_error)) Common::ErrorCodeValue::Enum Error;
        Common::ErrorCodeValue::Enum get_error() const { return error_; }

        __declspec (property(get=get_ConsistencyUnitDescriptions)) std::vector<ConsistencyUnitDescription> const& ConsistencyUnitDescriptions;
        std::vector<ConsistencyUnitDescription> const& get_ConsistencyUnitDescriptions() const { return consistencyUnitDescriptions_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("{0}", serviceDescription_);
            w.WriteLine("Error: {0}", error_);
            w.WriteLine("CUIDs: {0}", consistencyUnitDescriptions_);
        }

        FABRIC_FIELDS_03(serviceDescription_, error_, consistencyUnitDescriptions_);

    private:

        Reliability::ServiceDescription serviceDescription_;
        Common::ErrorCodeValue::Enum error_;
        std::vector<ConsistencyUnitDescription> consistencyUnitDescriptions_;
    };
}
