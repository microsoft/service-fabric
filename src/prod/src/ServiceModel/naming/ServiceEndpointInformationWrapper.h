// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceEndpointInformationWrapper
        : public Common::IFabricJsonSerializable
    {
    public:

        ServiceEndpointInformationWrapper()
            : kind_(FABRIC_SERVICE_ROLE_INVALID)
            , address_()
        {
        }
        
        ServiceEndpointInformationWrapper(
            FABRIC_SERVICE_ENDPOINT_ROLE kind,
            std::wstring const &address)
            : kind_(kind)
            , address_(address)
        {}

        __declspec(property(get=get_EndpointRole, put=put_EndpointRole)) FABRIC_SERVICE_ENDPOINT_ROLE Kind;
        __declspec(property(get=get_Address, put=put_Address)) std::wstring Address;

        FABRIC_SERVICE_ENDPOINT_ROLE get_EndpointRole() { return kind_; }
        std::wstring get_Address() { return address_; }

        void put_EndpointRole(FABRIC_SERVICE_ENDPOINT_ROLE kind) { kind_ = kind; }
        void put_Address(std::wstring const &address) { address_ = address; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Address, address_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_SERVICE_ENDPOINT_ROLE kind_;
        std::wstring address_;
    };
}
