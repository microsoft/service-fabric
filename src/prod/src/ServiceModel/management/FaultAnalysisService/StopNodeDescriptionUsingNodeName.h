// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StopNodeDescriptionUsingNodeName
            : public Serialization::FabricSerializable
        {
        public:

            StopNodeDescriptionUsingNodeName();
            StopNodeDescriptionUsingNodeName(StopNodeDescriptionUsingNodeName const &);
            StopNodeDescriptionUsingNodeName(StopNodeDescriptionUsingNodeName &&);

            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get = get_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID NodeInstanceId;

            std::wstring const & get_NodeName() const { return nodeName_; }
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return instanceId_; }

            Common::ErrorCode FromPublicApi(FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_STOP_NODE_DESCRIPTION_USING_NODE_NAME &) const;

            FABRIC_FIELDS_02(
                nodeName_,
                instanceId_);

        private:

            std::wstring nodeName_;
            FABRIC_NODE_INSTANCE_ID instanceId_;
        };
    }
}
