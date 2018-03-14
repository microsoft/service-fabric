// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StopNodeDescriptionInternal
            : public Serialization::FabricSerializable
        {
        public:
            DENY_COPY(StopNodeDescriptionInternal);
            
            StopNodeDescriptionInternal();
           // StopNodeDescriptionInternal(StopNodeDescriptionInternal const &);
            StopNodeDescriptionInternal(StopNodeDescriptionInternal &&);

            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get = get_NodeInstanceId)) FABRIC_NODE_INSTANCE_ID NodeInstanceId;
            __declspec(property(get = get_StopDurationInSeconds)) DWORD StopDurationInSeconds;

            std::wstring const & get_NodeName() const { return nodeName_; }
            FABRIC_NODE_INSTANCE_ID get_NodeInstanceId() const { return instanceId_; }
            DWORD get_StopDurationInSeconds() const { return stopDurationInSeconds_; }

            Common::ErrorCode FromPublicApi(FABRIC_STOP_NODE_DESCRIPTION_INTERNAL const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_STOP_NODE_DESCRIPTION_INTERNAL &) const;

            FABRIC_FIELDS_03(
                nodeName_,
                instanceId_,
                stopDurationInSeconds_);

        private:

            std::wstring nodeName_;
            FABRIC_NODE_INSTANCE_ID instanceId_;
            DWORD stopDurationInSeconds_;
        };
    }
}
