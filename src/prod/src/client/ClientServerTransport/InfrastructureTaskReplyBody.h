// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class InfrastructureTaskReplyBody : public Serialization::FabricSerializable
        {
        public:
            InfrastructureTaskReplyBody() 
                : complete_(false)
            {
            }

            InfrastructureTaskReplyBody(bool complete)
                : complete_(complete)
            {
            }

            __declspec(property(get=get_IsComplete)) bool IsComplete;
            bool get_IsComplete() const { return this->complete_; }

            FABRIC_FIELDS_01(complete_);

        private:
            bool complete_;
        };
    }
}

