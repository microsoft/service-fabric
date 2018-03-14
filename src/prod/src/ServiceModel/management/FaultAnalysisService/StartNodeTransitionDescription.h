// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartNodeTransitionDescription
            : public Serialization::FabricSerializable
        {
        public:

            StartNodeTransitionDescription();
            StartNodeTransitionDescription(StartNodeTransitionDescription const &);
            StartNodeTransitionDescription(StartNodeTransitionDescription &&);
            StartNodeTransitionDescription & operator = (StartNodeTransitionDescription && other);

            StartNodeTransitionDescription(
                FABRIC_NODE_TRANSITION_TYPE nodeTransitionType,
                Common::Guid const & operationId,
                std::wstring const & nodeName,
                uint64 nodeInstanceId,
                DWORD stopDurationInSeconds);
            
            __declspec(property(get = get_OperationId)) Common::Guid const & OperationId;
            __declspec(property(get = get_NodeTransitionType)) FABRIC_NODE_TRANSITION_TYPE NodeTransitionType;
            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get = get_NodeInstanceId)) uint64 NodeInstanceId;
            __declspec(property(get = get_StopDurationInSeconds)) DWORD StopDurationInSeconds;

            Common::Guid const & get_OperationId() const { return operationId_; }
            FABRIC_NODE_TRANSITION_TYPE  get_NodeTransitionType() const { return nodeTransitionType_; }
            std::wstring const & get_NodeName() const { return nodeName_; }
            uint64 get_NodeInstanceId() const { return instanceId_; }
            DWORD get_StopDurationInSeconds() const { return stopDurationInSeconds_; }
            
            Common::ErrorCode FromPublicApi(FABRIC_NODE_TRANSITION_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_NODE_TRANSITION_DESCRIPTION &) const;

            FABRIC_FIELDS_05(
                nodeTransitionType_,
                operationId_,
                nodeName_,
                instanceId_,
                stopDurationInSeconds_);

        private:
            FABRIC_NODE_TRANSITION_TYPE nodeTransitionType_;
            Common::Guid operationId_;

            std::wstring nodeName_;
            uint64 instanceId_;

            // stop only
            DWORD stopDurationInSeconds_;
        };
    }
}
