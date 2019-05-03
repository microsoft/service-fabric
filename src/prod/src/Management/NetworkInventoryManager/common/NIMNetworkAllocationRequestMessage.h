// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to request network pool range for a specific netowork.
        class NetworkAllocationRequestMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkAllocationRequestMessage()
            {}
                
            NetworkAllocationRequestMessage(
                const std::wstring &networkName,
                const std::wstring &nodeName,
                const std::wstring &infraIpAddress) :
                networkName_(networkName),
                nodeName_(nodeName),
                infraIpAddress_(infraIpAddress)
            {
            }
            
            __declspec (property(get = get_NetworkName, put = set_NetworkName)) const std::wstring NetworkName;
            const std::wstring & get_NetworkName() const { return networkName_; }
            void set_NetworkName(const std::wstring & value) { networkName_ = value; }

            __declspec (property(get = get_NodeName, put = set_NodeName)) const std::wstring NodeName;
            const std::wstring & get_NodeName() const { return nodeName_; }
            void set_NodeName(const std::wstring & value) { nodeName_ = value; }


            __declspec(property(get = get_InfraIpAddress, put = set_InfraIpAddress)) const std::wstring InfraIpAddress;
            const std::wstring & get_InfraIpAddress() const { return infraIpAddress_; }
            void set_InfraIpAddress(const std::wstring & value) { infraIpAddress_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            static const std::wstring ActionName;
            FABRIC_FIELDS_03(networkName_, nodeName_, infraIpAddress_);

        private:
            std::wstring networkName_;
            std::wstring nodeName_;
            std::wstring infraIpAddress_;

        };
    }
}