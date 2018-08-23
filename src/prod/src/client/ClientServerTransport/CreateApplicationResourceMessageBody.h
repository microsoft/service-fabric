//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateApplicationResourceMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:

            CreateApplicationResourceMessageBody() = default;

            CreateApplicationResourceMessageBody(ServiceModel::ModelV2::ApplicationDescription && description)
                : description_(move(description))
            {
            }

            FABRIC_FIELDS_01(description_);

            std::wstring const & get_ApplicationName() const { return description_.Name; }
            __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;

            ServiceModel::ModelV2::ApplicationDescription && TakeDescription() { return std::move(description_); }

        private:
            ServiceModel::ModelV2::ApplicationDescription description_;
        };
    }
}
