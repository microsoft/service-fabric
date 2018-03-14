// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class NodeDoesNotMatchFaultBody : public Serialization::FabricSerializable
    {
    public:
        NodeDoesNotMatchFaultBody()
        {
        }

        explicit NodeDoesNotMatchFaultBody(NodeInstance const & nodeInstance, std::wstring const & ringName)
            : nodeInstance_(nodeInstance), ringName_(ringName)
        {
        }

        __declspec (property(get=get_NodeInstance)) NodeInstance const & Instance;
        NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

        __declspec (property(get=get_RingName)) std::wstring const & RingName;
        std::wstring const & get_RingName() const { return ringName_; }

        FABRIC_FIELDS_02(nodeInstance_, ringName_);

    private:
        NodeInstance nodeInstance_;
        std::wstring ringName_;
    };
}
