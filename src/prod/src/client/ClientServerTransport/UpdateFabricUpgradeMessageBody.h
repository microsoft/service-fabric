// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class UpdateFabricUpgradeMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateFabricUpgradeMessageBody()
            {
            }

            explicit UpdateFabricUpgradeMessageBody(FabricUpgradeUpdateDescription const& description)
                : updateDescription_(description)
            {
            }

            explicit UpdateFabricUpgradeMessageBody(FabricUpgradeUpdateDescription && description)
                : updateDescription_(std::move(description))
            {
            }

            __declspec(property(get=get_UpdateDescription)) FabricUpgradeUpdateDescription const & UpdateDescription;
            FabricUpgradeUpdateDescription const & get_UpdateDescription() const { return updateDescription_; }

            FABRIC_FIELDS_01(updateDescription_);

        private:
            FabricUpgradeUpdateDescription updateDescription_;
        };
    }
}
