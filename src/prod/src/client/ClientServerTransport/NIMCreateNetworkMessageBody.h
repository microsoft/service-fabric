// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class CreateNetworkMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            CreateNetworkMessageBody()
            {
            }

            CreateNetworkMessageBody(ServiceModel::ModelV2::NetworkResourceDescription const & networkDescription)
                : networkDescription_(networkDescription)
            {
            }

            __declspec(property(get = get_NetworkDescription)) ServiceModel::ModelV2::NetworkResourceDescription const & NetworkDescription;

            ServiceModel::ModelV2::NetworkResourceDescription const & get_NetworkDescription() const { return networkDescription_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
            {
                w.WriteLine("CreateNetworkMessageBody: Network: [{0}], Type: {1}, Subnet: [{2}]", 
                    networkDescription_.Name,
                    static_cast<int>(networkDescription_.NetworkType),
                    networkDescription_.NetworkAddressPrefix);
            }

            FABRIC_FIELDS_01(networkDescription_);

        private:
            ServiceModel::ModelV2::NetworkResourceDescription networkDescription_;
        };
    }
}