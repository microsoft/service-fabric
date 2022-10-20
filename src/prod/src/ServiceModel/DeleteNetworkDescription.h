// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeleteNetworkDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeleteNetworkDescription();

        DeleteNetworkDescription(std::wstring const & networkName);

        DeleteNetworkDescription(DeleteNetworkDescription const & otherDeleteNetworkDescription);

        ~DeleteNetworkDescription() {};       

        Common::ErrorCode FromPublicApi(__in FABRIC_DELETE_NETWORK_DESCRIPTION const & deleteDescription);

        __declspec(property(get = get_NetworkName)) std::wstring const &NetworkName;

        std::wstring const& get_NetworkName() const { return networkName_; }

        FABRIC_FIELDS_01(networkName_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(Constants::NetworkName, networkName_)            
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:
        std::wstring networkName_;
    };
}
