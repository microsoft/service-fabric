// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class UpdateApplicationUpgradeMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateApplicationUpgradeMessageBody()
            {
            }

            explicit UpdateApplicationUpgradeMessageBody(ApplicationUpgradeUpdateDescription const& description)
                : updateDescription_(description)
            {
            }

            explicit UpdateApplicationUpgradeMessageBody(ApplicationUpgradeUpdateDescription && description)
                : updateDescription_(std::move(description))
            {
            }

            __declspec(property(get=get_UpdateDescription)) ApplicationUpgradeUpdateDescription const & UpdateDescription;
            ApplicationUpgradeUpdateDescription const & get_UpdateDescription() const { return updateDescription_; }

            FABRIC_FIELDS_01(updateDescription_);

        private:
            ApplicationUpgradeUpdateDescription updateDescription_;
        };
    }
}
