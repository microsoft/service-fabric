// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceMonitorServiceRegistrationResponse : public Serialization::FabricSerializable
        {
        public:

            ResourceMonitorServiceRegistrationResponse() = default;

            ResourceMonitorServiceRegistrationResponse(std::wstring const & fabricHostAddress);

            __declspec(property(get=get_FabricHostAddress)) std::wstring const & FabricHostAddress;
            std::wstring const & get_FabricHostAddress() const { return fabricHostAddress_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(fabricHostAddress_);

        private:
            std::wstring fabricHostAddress_;
        };
    }
}
