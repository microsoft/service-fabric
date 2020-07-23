// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to enumerate network definitions.
        class NetworkEnumerateRequestMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkEnumerateRequestMessage()
            {}
                
            NetworkEnumerateRequestMessage(
                const std::wstring &networkNameFilter) :
                networkNameFilter_(networkNameFilter)
            {
            }
            
            __declspec (property(get = get_NetworkNameFilter, put = set_NetworkNameFilter)) const std::wstring NetworkNameFilter;
            const std::wstring & get_NetworkNameFilter() const { return networkNameFilter_; }
            void set_NetworkNameFilter(const std::wstring & value) { networkNameFilter_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            
            static const std::wstring ActionName;
            FABRIC_FIELDS_01(networkNameFilter_);

        private:
            std::wstring networkNameFilter_;

        };
    }
}