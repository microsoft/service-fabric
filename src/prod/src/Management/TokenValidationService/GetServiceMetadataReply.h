// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class GetServiceMetadataReply : public Serialization::FabricSerializable
        {
        public:
            GetServiceMetadataReply();
            GetServiceMetadataReply(
                ServiceModel::TokenValidationServiceMetadata const & metadata,
                Common::ErrorCode const & error);

            __declspec(property(get=get_ServiceMetadata)) ServiceModel::TokenValidationServiceMetadata const & ServiceMetadata;
            ServiceModel::TokenValidationServiceMetadata const & get_ServiceMetadata() const { return metadata_; }

            __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
            Common::ErrorCode const & get_Error() const { return error_; }

            FABRIC_FIELDS_02(metadata_, error_);

        private:
            ServiceModel::TokenValidationServiceMetadata metadata_;
            Common::ErrorCode error_;
        };
    }
}
