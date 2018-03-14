// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class NodeResult : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(NodeResult)
        
        public:

            NodeResult();
            NodeResult(std::wstring const & nodeName, FABRIC_NODE_INSTANCE_ID const nodeInstance): nodeName_(nodeName), nodeInstanceId_(nodeInstance) {};
            NodeResult(NodeResult &&);

            __declspec(property(get = get_NodeName)) std::wstring const& NodeName;
            std::wstring const& get_NodeName() const { return nodeName_; }

            __declspec(property(get = get_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID  NodeInstanceId;
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return nodeInstanceId_; }

            Common::ErrorCode FromPublicApi(FABRIC_NODE_RESULT const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_NODE_RESULT &) const;

            FABRIC_FIELDS_02(nodeName_, nodeInstanceId_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeName, nodeName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeInstanceId, nodeInstanceId_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nodeName_;
            FABRIC_NODE_INSTANCE_ID nodeInstanceId_;
        };
    }
}
