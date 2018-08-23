//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class IndependentVolumeRef
        : public ContainerVolumeRef
        {
        public:
            IndependentVolumeRef() = default;

            bool operator==(IndependentVolumeRef const & other) const;
            bool operator!=(IndependentVolumeRef const & other) const;

            // Used for unit test
            IndependentVolumeRef(std::wstring const &name, std::wstring const &destinationPath)
            {
                name_ = name;
                destinationPath_ = destinationPath;
            }

            __declspec(property(get=get_VolumeDescriptionSPtr, put=put_VolumeDescriptionSPtr)) std::shared_ptr<Management::ClusterManager::VolumeDescription> const & VolumeDescriptionSPtr;
            std::shared_ptr<Management::ClusterManager::VolumeDescription> const & get_VolumeDescriptionSPtr() const { return volumeDescriptionSPtr_; }
            void put_VolumeDescriptionSPtr(std::shared_ptr<Management::ClusterManager::VolumeDescription> const & value) { volumeDescriptionSPtr_ = value; }

            void Test_SetDestinationPath(std::wstring const &path) { destinationPath_ = path; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN();
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::volumeDescriptionForImageBuilder, volumeDescriptionSPtr_, volumeDescriptionSPtr_ != nullptr)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN();
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumeDescriptionSPtr_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_01(volumeDescriptionSPtr_);

        private:
            // This is constructed to pass volumeDescription to IB, shall not return to users and not persisted.
            std::shared_ptr<Management::ClusterManager::VolumeDescription> volumeDescriptionSPtr_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::IndependentVolumeRef);
