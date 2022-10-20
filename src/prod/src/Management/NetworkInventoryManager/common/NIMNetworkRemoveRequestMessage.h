// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to request message loop test.
        class NetworkRemoveRequestMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkRemoveRequestMessage()
            {}

            NetworkRemoveRequestMessage(
                const std::wstring & networkId)
                : networkId_(networkId)
            {
            }

            NetworkRemoveRequestMessage(
                const std::wstring & networkId,
                const std::wstring & nodeId)
                : networkId_(networkId),
                nodeId_(nodeId)
            {
            }
            
            __declspec(property(get=get_NetworkId)) const std::wstring & NetworkId;
            const std::wstring & get_NetworkId() const { return networkId_; }

            __declspec(property(get = get_NodeId)) const std::wstring & NodeId;
            const std::wstring & get_NodeId() const { return nodeId_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            static const std::wstring ActionName;

            FABRIC_FIELDS_02(networkId_, nodeId_);

        private:
            std::wstring networkId_;
            std::wstring nodeId_;
        };
    }
}

