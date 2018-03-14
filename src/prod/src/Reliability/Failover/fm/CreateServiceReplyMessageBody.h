// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class CreateServiceReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        CreateServiceReplyMessageBody()
        {
        }

        CreateServiceReplyMessageBody(
            Common::ErrorCodeValue::Enum errorCodeValue,
            Reliability::ServiceDescription serviceDescription,
            std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions = std::vector<ConsistencyUnitDescription>())
            : errorCodeValue_(errorCodeValue),
              serviceDescription_(serviceDescription),
              consistencyUnitDescriptions_(std::move(consistencyUnitDescriptions))
        {
        }

        __declspec (property(get=get_ErrorCodeValue)) Common::ErrorCodeValue::Enum ErrorCodeValue;
        Common::ErrorCodeValue::Enum get_ErrorCodeValue() const { return errorCodeValue_; }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec (property(get=get_ConsistencyUnitDescriptions)) std::vector<ConsistencyUnitDescription> const& ConsistencyUnitDescriptions;
        std::vector<ConsistencyUnitDescription> const& get_ConsistencyUnitDescriptions() const { return consistencyUnitDescriptions_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("Error: {0}", errorCodeValue_);

            w.WriteLine(serviceDescription_);

            for (size_t i = 0; i < consistencyUnitDescriptions_.size(); i++)
            {
                w.WriteLine(consistencyUnitDescriptions_[i]);
            }
        }

        FABRIC_FIELDS_03(errorCodeValue_, serviceDescription_, consistencyUnitDescriptions_);

    private:

        Common::ErrorCodeValue::Enum errorCodeValue_;
        Reliability::ServiceDescription serviceDescription_;
        std::vector<ConsistencyUnitDescription> consistencyUnitDescriptions_;
    };
}
