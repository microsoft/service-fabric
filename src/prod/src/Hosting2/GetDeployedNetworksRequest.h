// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetDeployedNetworksRequest : public Serialization::FabricSerializable
    {
    public:

        GetDeployedNetworksRequest();
        GetDeployedNetworksRequest(ServiceModel::NetworkType::Enum networkType);

        __declspec(property(get = get_NetworkType)) ServiceModel::NetworkType::Enum const & NetworkType;
        ServiceModel::NetworkType::Enum  const & get_NetworkType() const { return networkType_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(networkType_);

    private:

        ServiceModel::NetworkType::Enum networkType_;
    };
}
