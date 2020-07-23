// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class ValidateNetworkMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ValidateNetworkMessageBody()
            {
            }

            ValidateNetworkMessageBody(
                std::vector<std::wstring> const & networks)
                : networks_(networks)
            {
            }

            __declspec(property(get = get_Networks)) std::vector<std::wstring> const & Networks;

            std::vector<std::wstring> const & get_Networks() const { return networks_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
            {
                w.WriteLine("ValidateNetworkMessageBody: NetworkCount: {0}", networks_.size());
            }

            FABRIC_FIELDS_01(networks_);

        private:
            std::vector<std::wstring> networks_;
        };
    }
}