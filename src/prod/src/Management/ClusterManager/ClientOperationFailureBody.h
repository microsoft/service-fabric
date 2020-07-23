// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class ClientOperationFailureBody : public Serialization::FabricSerializable
        {
        public:
            ClientOperationFailureBody() 
            {
            }

            ClientOperationFailureBody(Common::ErrorCode const & error) 
                : error_(error.ReadValue())
            {
            }

            __declspec(property(get=get_Error)) Common::ErrorCode Error;
            Common::ErrorCode get_Error() const { return this->error_; }

            FABRIC_FIELDS_01(error_);

        private:
            Common::ErrorCodeValue::Enum error_;
        };
    }
}
