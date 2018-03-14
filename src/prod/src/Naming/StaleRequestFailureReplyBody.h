// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StaleRequestFailureReplyBody : public Serialization::FabricSerializable
    {
    public:
        StaleRequestFailureReplyBody() : instance_(0) { }

        explicit StaleRequestFailureReplyBody(__int64 instance) : instance_(instance) { }

        __declspec(property(get=get_Instance)) __int64 Instance;
        __int64 get_Instance() const { return instance_; }

        FABRIC_FIELDS_01(instance_);

    private:
        __int64 instance_;
    };
}

