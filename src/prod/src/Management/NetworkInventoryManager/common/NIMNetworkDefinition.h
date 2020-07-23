// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NetworkAllocationRequestMessage;

        //--------
        // This class defines a "Network", consisting of a subnet (only one supported right now)
        // gateway and the overlay id.
        class NetworkDefinition : public Serialization::FabricSerializable
        {
        public:

            NetworkDefinition()
            {
                this->instanceID_ = Common::Guid::NewGuid().ToString();
            }

            NetworkDefinition(
                std::wstring const &networkName,
                std::wstring const &networkId,
                std::wstring &subnet,
                int subnetPrefix,
                std::wstring &gateway,
                int overlayId) :	
                    networkName_(networkName),
                    networkId_(networkId),
                    subnet_(subnet),
                    subnetPrefix_(subnetPrefix),
                    gateway_(gateway),
                    overlayId_(overlayId)
            {
                this->instanceID_ = Common::Guid::NewGuid().ToString();
            }

            virtual ~NetworkDefinition()
            {}

            std::shared_ptr<NetworkDefinition> Clone()
            {
                auto networkDefinition = make_shared<NetworkDefinition>(this->networkName_,
                    this->networkId_,
                    this->subnet_,
                    this->subnetPrefix_,
                    this->gateway_,
                    this->overlayId_);

                return networkDefinition;
            }

            // Get the name of the overlay network.
            __declspec(property(get = get_NetworkName, put = set_NetworkName)) std::wstring NetworkName;
            std::wstring get_NetworkName() const { return this->networkName_; };
            void set_NetworkName(const std::wstring & value) { this->networkName_ = value; }

            // Get the id of the overlay network.
            __declspec(property(get = get_NetworkId, put = set_NetworkId)) std::wstring NetworkId;
            std::wstring get_NetworkId() const { return this->networkId_; };
            void set_NetworkId(const std::wstring & value) { this->networkId_ = value; }

            // Get the volatile object instance ID.
            __declspec(property(get = get_InstanceID, put = set_InstanceID)) std::wstring InstanceID;
            std::wstring get_InstanceID() const { return this->instanceID_; };
            void set_InstanceID(const std::wstring & value) { this->instanceID_ = value; }

            // Get the subnet of the overlay network.
            __declspec(property(get = get_Subnet, put = set_Subnet)) std::wstring Subnet;
            std::wstring get_Subnet() const { return this->subnet_; };
            void set_Subnet(const std::wstring & value) { this->subnet_ = value; }

            // Get the prefix of the overlay network.
            __declspec(property(get = get_Prefix, put = set_Prefix)) int Prefix;
            int get_Prefix() const { return this->subnetPrefix_; };
            void set_Prefix(int value) { this->subnetPrefix_ = value; }

            // Get the gateway of the overlay network.
            __declspec(property(get = get_Gateway, put = set_Gateway)) std::wstring Gateway;
            std::wstring get_Gateway() const { return this->gateway_; };
            void set_Gateway(const std::wstring & value) { this->gateway_ = value; }

            // Get the vxlan id of the overlay network.
            __declspec(property(get = get_OverlayId, put = set_OverlayId)) int OverlayId;
            int get_OverlayId() const { return this->overlayId_; };
            void set_OverlayId(int value) { this->overlayId_ = value; }

            FABRIC_FIELDS_06(networkName_, networkId_, subnet_, gateway_, subnetPrefix_, overlayId_);


        private:

            // Network name
            std::wstring networkName_;

            // Network id
            std::wstring networkId_;

            // Instance id
            std::wstring instanceID_;

            // Subnet in CIDR format
            std::wstring subnet_;

            // Gateway in dot format
            std::wstring gateway_;

            // Subnet prefix
            int subnetPrefix_;

            // Overlay id
            int overlayId_;
        };
        
        typedef std::shared_ptr<NetworkDefinition> NetworkDefinitionSPtr;
    }
}
