// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NodeStateRemovedMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        NodeStateRemovedMessageBody()
            : nodeName_()
            , nodeId_(Federation::NodeId::MinNodeId)
        {
        }

        explicit NodeStateRemovedMessageBody(std::wstring const& value)
            : nodeName_(value)
            , nodeId_(Federation::NodeId::MinNodeId)
        {
        }

        __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring get_NodeName() const { return nodeName_; }

        FABRIC_FIELDS_02(nodeId_, nodeName_);

    private:
        
        //
        // NOTE: Both nodeId and nodeName are required in this object to be compatible
        // with V1 messages from clients. V1 clients convert nodeName to nodeId and just 
        // populate the nodeId field. So when using this message, nodeId should be used 
        // only if the nodeName field is empty.
        //
        Federation::NodeId nodeId_;
        std::wstring nodeName_;
    };
}
