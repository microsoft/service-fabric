// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class ICancelUpgradeContext
            {
                DENY_COPY(ICancelUpgradeContext);

            public:

                __declspec(property(get=get_InstanceId)) uint64 InstanceId;
                uint64 get_InstanceId() const { return instanceId_; }
            
                virtual void SendReply() = 0;

                virtual ~ICancelUpgradeContext()
                {
                }

            protected:
                ICancelUpgradeContext(uint64 instanceId)
                : instanceId_(instanceId)
                {
                }

            private:
                uint64 const instanceId_;
            };

            class CancelFabricUpgradeContext : public ICancelUpgradeContext
            {
                DENY_COPY(CancelFabricUpgradeContext);
            public:
                CancelFabricUpgradeContext(ServiceModel::FabricUpgradeSpecification && upgradeSpecification, ReconfigurationAgent & ra);

                void SendReply();

                static ICancelUpgradeContextSPtr Create(ServiceModel::FabricUpgradeSpecification && upgradeSpecification, ReconfigurationAgent & ra);

            private:
                ReconfigurationAgent & ra_;
                ServiceModel::FabricUpgradeSpecification const upgradeSpecification_;
            };

            class CancelApplicationUpgradeContext : public ICancelUpgradeContext
            {
                DENY_COPY(CancelApplicationUpgradeContext);

            public:
                CancelApplicationUpgradeContext(UpgradeDescription && upgradeDescription, ReconfigurationAgent & ra);

                void SendReply();

                static ICancelUpgradeContextSPtr Create(UpgradeDescription && upgradeDescription, ReconfigurationAgent & ra);

            private:
                ReconfigurationAgent & ra_;
                UpgradeDescription const upgradeDescription_;
            };
        }
    }
}


