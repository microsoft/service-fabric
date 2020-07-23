// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "NIMNetworkDefinition.h"

namespace Management
{
    namespace NetworkInventoryManager
    {
        //--------
        // This class is the response for NetworkEnumerateRequestMessage call.
        class NetworkEnumerateResponseMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkEnumerateResponseMessage()
            {}

            // Get the network enumeration
            __declspec(property(get = get_NetworkDefinitionList, put = set_NetworkDefinitionList)) std::vector<NetworkDefinitionSPtr> & NetworkDefinitionList;
            std::vector<NetworkDefinitionSPtr> & get_NetworkDefinitionList() { return this->networkDefinitionList_; }
            void set_NetworkDefinitionList(const std::vector<NetworkDefinitionSPtr> & value) { this->networkDefinitionList_ = value; }

            __declspec(property(get = get_ErrorCode, put = set_ErrorCode)) Common::ErrorCode ErrorCode;
            Common::ErrorCode get_ErrorCode() const { return error_; }
            void set_ErrorCode(const Common::ErrorCode value) { error_ = value; }

            FABRIC_FIELDS_02(networkDefinitionList_, error_);

        private:

            // NetworkDefinition list.
            std::vector<NetworkDefinitionSPtr> networkDefinitionList_;
            Common::ErrorCode error_;
        };
    }
}