// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeServiceIpcInitResponseMessageBody 
        : public Serialization::FabricSerializable
    {
    public:
        EntreeServiceIpcInitResponseMessageBody() {}

        EntreeServiceIpcInitResponseMessageBody(
            Federation::NodeInstance const &instance,
            std::wstring const &nodeName)
            : instance_(instance)
            , nodeName_(nodeName)
        {
        }

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const &NodeInstance;
        Federation::NodeInstance const& get_NodeInstance() const { return instance_; }

        __declspec(property(get=get_NodeName)) std::wstring const &NodeName;
        std::wstring  const& get_NodeName() const { return nodeName_; }

        FABRIC_FIELDS_02(instance_, nodeName_);
    private:
        Federation::NodeInstance instance_;
        std::wstring nodeName_;
    };
}
