// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class UpdateApplicationMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateApplicationMessageBody()
            {
            }

            UpdateApplicationMessageBody(ServiceModel::ApplicationUpdateDescriptionWrapper const & description)
                : updateDescription_(description)
            {
            }

            __declspec(property(get = get_UpdateDescription)) ServiceModel::ApplicationUpdateDescriptionWrapper const & UpdateDescription;
            ServiceModel::ApplicationUpdateDescriptionWrapper const & get_UpdateDescription() { return updateDescription_; }

            FABRIC_FIELDS_01(updateDescription_);

        private:
            ServiceModel::ApplicationUpdateDescriptionWrapper updateDescription_;
        };
    }
}
