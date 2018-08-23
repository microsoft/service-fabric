// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ModelV2
    {
        class VolumeQueryResult
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
            , public IPageContinuationToken
        {
            DENY_COPY(VolumeQueryResult)

        public:
            VolumeQueryResult() = default;
            VolumeQueryResult(VolumeQueryResult &&) = default;
            VolumeQueryResult& operator=(VolumeQueryResult &&) = default;
            VolumeQueryResult(Management::ClusterManager::VolumeDescriptionSPtr const & volumeDescription);
            std::wstring CreateContinuationToken() const override;

            __declspec(property(get=get_VolumeName)) std::wstring const & VolumeName;
            std::wstring const & get_VolumeName() const { return volumeName_; }

            __declspec(property(get=get_VolumeDescription)) Management::ClusterManager::VolumeDescriptionSPtr const & VolumeDescription;
            Management::ClusterManager::VolumeDescriptionSPtr const & get_VolumeDescription() const { return volumeDescription_; }

            FABRIC_FIELDS_02(volumeName_, volumeDescription_)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(Constants::nameCamelCase, volumeName_)
                SERIALIZABLE_PROPERTY(Constants::properties, volumeDescription_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumeName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumeDescription_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            std::wstring volumeName_;
            Management::ClusterManager::VolumeDescriptionSPtr volumeDescription_;
        };

        QUERY_JSON_LIST(VolumeQueryResultList, VolumeQueryResult)
    }
}
