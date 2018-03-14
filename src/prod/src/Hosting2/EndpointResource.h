// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class EndpointResource : public EnvironmentResource
    {
        DENY_COPY(EndpointResource)

    public:
        explicit EndpointResource(
            ServiceModel::EndpointDescription const & endpointDescription,
            ServiceModel::EndpointBindingPolicyDescription const & endpointBindingPolicy);
        virtual ~EndpointResource();

        __declspec (property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return this->endpointDescription_.Name; }

        __declspec (property(get=get_Type)) ServiceModel::EndpointType::Enum Type;
        ServiceModel::EndpointType::Enum get_Type() const { return this->endpointDescription_.Type; }

        __declspec (property(get=get_Protocol)) ServiceModel::ProtocolType::Enum Protocol;
        ServiceModel::ProtocolType::Enum get_Protocol() const { return this->endpointDescription_.Protocol; }

        __declspec(property(get=get_Port, put=set_Port)) int Port;
        int get_Port() const { return endpointDescription_.Port; }
        void set_Port(int value) { endpointDescription_.Port = value; }

        __declspec(property(get = get_IpAddressOrFqdn, put = set_IpAddressOrFqdn)) std::wstring const & IpAddressOrFqdn;
        std::wstring const & get_IpAddressOrFqdn() const { return endpointDescription_.IpAddressOrFqdn; }
        void set_IpAddressOrFqdn(std::wstring const & value) { endpointDescription_.IpAddressOrFqdn = value; }
        
        __declspec (property(get=get_EndpointDescription)) ServiceModel::EndpointDescription EndpointDescriptionObj;
        ServiceModel::EndpointDescription const & get_EndpointDescription() const { return this->endpointDescription_; }

        __declspec (property(get = get_EndpointBindingPolicy)) ServiceModel::EndpointBindingPolicyDescription EndPointBindingPolicy;
        ServiceModel::EndpointBindingPolicyDescription const & get_EndpointBindingPolicy() const { return this->endpointBindingPolicy_; }
     
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        ServiceModel::EndpointDescription endpointDescription_;
        ServiceModel::EndpointBindingPolicyDescription endpointBindingPolicy_;
   };
}
