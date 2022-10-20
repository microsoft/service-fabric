// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to create a new network definition.
        class NetworkCreateRequestMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkCreateRequestMessage()
            {}
                
            NetworkCreateRequestMessage(
                const std::wstring &networkName,
                const NetworkDefinitionSPtr networkDefinition) :
                networkName_(networkName),
                networkDefinition_(networkDefinition)
            {
            }
            
            __declspec (property(get = get_NetworkName, put = set_NetworkName)) const std::wstring NetworkName;
            const std::wstring & get_NetworkName() const { return networkName_; }
            void set_NetworkName(const std::wstring & value) { networkName_ = value; }

            __declspec(property(get = get_NetworkDefinition, put = set_NetworkDefinition)) NetworkDefinitionSPtr NetworkDefinition;
            NetworkDefinitionSPtr get_NetworkDefinition() const { return networkDefinition_; }
            void set_NetworkDefinition(const NetworkDefinitionSPtr value) { networkDefinition_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            static const std::wstring ActionName;
            FABRIC_FIELDS_02(networkName_, networkDefinition_);

        private:
            std::wstring networkName_;
            NetworkDefinitionSPtr networkDefinition_;
        };
    }
}