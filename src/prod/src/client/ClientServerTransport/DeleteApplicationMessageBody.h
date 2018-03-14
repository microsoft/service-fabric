// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteApplicationMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteApplicationMessageBody() : description_() { }

            DeleteApplicationMessageBody(
                ServiceModel::DeleteApplicationDescription const & description)
                : description_(description)
            {
            }

            __declspec(property(get=get_Description)) ServiceModel::DeleteApplicationDescription const & Description;
            ServiceModel::DeleteApplicationDescription const & get_Description() const { return description_; }

            FABRIC_FIELDS_01(description_);

        private:
            ServiceModel::DeleteApplicationDescription description_;
        };
    }
}
