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
        // This class is the response for NetworkCreateRequestMessage call.
        class NetworkCreateResponseMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkCreateResponseMessage()
            {}

            // Get the network definition
            __declspec(property(get = get_NetworkDefinition, put = set_NetworkDefinition)) NetworkDefinitionSPtr & NetworkDefinition;
            NetworkDefinitionSPtr & get_NetworkDefinition() { return this->networkDefinition_; }
            void set_NetworkDefinition(const NetworkDefinitionSPtr & value) { this->networkDefinition_ = value; }

            __declspec(property(get = get_ErrorCode, put = set_ErrorCode)) Common::ErrorCode ErrorCode;
            Common::ErrorCode get_ErrorCode() const { return error_; }
            void set_ErrorCode(const Common::ErrorCode value) { error_ = value; }

            FABRIC_FIELDS_02(networkDefinition_, error_);

        private:

            // NetworkDefinition.
            NetworkDefinitionSPtr networkDefinition_;
            Common::ErrorCode error_;
        };
    }
}