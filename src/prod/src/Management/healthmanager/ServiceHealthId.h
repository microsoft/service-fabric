// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ServiceHealthId : public Serialization::FabricSerializable
        {
        public:
            ServiceHealthId();

            explicit ServiceHealthId(
                std::wstring const & serviceName);

            explicit ServiceHealthId(
                std::wstring && serviceName);

            ServiceHealthId(ServiceHealthId const & other);
            ServiceHealthId & operator = (ServiceHealthId const & other);

            ServiceHealthId(ServiceHealthId && other);
            ServiceHealthId & operator = (ServiceHealthId && other);
            
            ~ServiceHealthId();
            
            __declspec (property(get=get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }
            
            bool operator == (ServiceHealthId const & other) const;
            bool operator != (ServiceHealthId const & other) const;
            bool operator < (ServiceHealthId const & other) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_01(serviceName_);
            
        private:
            std::wstring serviceName_;
        };
    }
}
