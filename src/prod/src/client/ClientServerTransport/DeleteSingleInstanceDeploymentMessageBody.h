//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteSingleInstanceDeploymentMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteSingleInstanceDeploymentMessageBody() = default;

            DeleteSingleInstanceDeploymentMessageBody(
                ServiceModel::DeleteSingleInstanceDeploymentDescription const & description)
                : description_(description)
            {
            }

            __declspec(property(get = get_Description)) ServiceModel::DeleteSingleInstanceDeploymentDescription const & Description;
            ServiceModel::DeleteSingleInstanceDeploymentDescription const & get_Description() const { return description_; }

            FABRIC_FIELDS_01(description_);

        private:
            ServiceModel::DeleteSingleInstanceDeploymentDescription description_;
        };
    }
}
