// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceCorrelationDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceCorrelationDescription();

        ServiceCorrelationDescription(std::wstring const& serviceName, FABRIC_SERVICE_CORRELATION_SCHEME scheme);

        ServiceCorrelationDescription(ServiceCorrelationDescription const& other);

        ServiceCorrelationDescription(ServiceCorrelationDescription && other);

        ServiceCorrelationDescription & operator = (ServiceCorrelationDescription const& other);

        ServiceCorrelationDescription & operator = (ServiceCorrelationDescription && other);

        bool operator == (ServiceCorrelationDescription const & other) const;

        __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        __declspec (property(get=get_Scheme)) FABRIC_SERVICE_CORRELATION_SCHEME Scheme;
        FABRIC_SERVICE_CORRELATION_SCHEME get_Scheme() const { return scheme_; }

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(serviceName_, scheme_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Scheme, scheme_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        std::wstring serviceName_;
        FABRIC_SERVICE_CORRELATION_SCHEME scheme_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServiceCorrelationDescription);
