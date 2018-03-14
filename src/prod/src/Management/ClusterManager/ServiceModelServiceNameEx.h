// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ClusterManager
    {
        struct ServiceModelServiceNameEx : public ServiceModelServiceName
        {
        public:
            ServiceModelServiceNameEx() : dnsName_() { }
            ServiceModelServiceNameEx(Common::NamingUri const & name, std::wstring const & dnsName) 
                : ServiceModelServiceName(name.ToString()),
                dnsName_(dnsName)
            {
            }

            explicit ServiceModelServiceNameEx(ServiceModelServiceNameEx const & other)
                : ServiceModelServiceName(other.value_),
                dnsName_(other.dnsName_)
            {
            }

            explicit ServiceModelServiceNameEx(ServiceModelServiceNameEx && other)
                : ServiceModelServiceName(move(other.value_)),
                dnsName_(move(other.dnsName_))
            {
            }

            ServiceModelServiceNameEx const & operator = (ServiceModelServiceNameEx && other)
            {
                if (this != &other)
                {
                    value_ = move(other.value_);
                    dnsName_ = move(other.dnsName_);
                }

                return *this;
            }

            // DNS name is intentionally left out of the comparison, the identity of the service is always
            // uniquely determined by the serviceName. Scenario where one service can have multiple DNS names is unsupported. 
            bool operator == (ServiceModelServiceNameEx const & other) const { return Name == other.Name; }
            bool operator != (ServiceModelServiceNameEx const & other) const { return !(*this == other); }

            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return Value; }

            __declspec(property(get = get_DnsName)) std::wstring const & DnsName;
            std::wstring const & get_DnsName() const { return dnsName_; }

            FABRIC_FIELDS_01(dnsName_);

        private:
            std::wstring dnsName_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ClusterManager::ServiceModelServiceNameEx);
