// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceMonitorServiceRegistration : public Serialization::FabricSerializable
        {
        public:

            static Common::GlobalWString ResourceMonitorServiceRegistrationAction;

            ResourceMonitorServiceRegistration() = default;

            ResourceMonitorServiceRegistration(std::wstring const & hostId);

            __declspec(property(get=get_HostId)) std::wstring const & Id;
            std::wstring const & get_HostId() const { return id_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(id_);

        private:
            std::wstring id_;
        };
    }
}
