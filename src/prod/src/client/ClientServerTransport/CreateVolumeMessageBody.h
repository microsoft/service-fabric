//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class CreateVolumeMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        CreateVolumeMessageBody() = default;

        CreateVolumeMessageBody(
            Management::ClusterManager::VolumeDescriptionSPtr const & description)
            : volumeDescription_(description)
        {
        }

        __declspec(property(get=get_VolumeDescription)) Management::ClusterManager::VolumeDescriptionSPtr const & VolumeDescription;
        Management::ClusterManager::VolumeDescriptionSPtr const & get_VolumeDescription() const { return volumeDescription_; }

        FABRIC_FIELDS_01(volumeDescription_);

    private:
        Management::ClusterManager::VolumeDescriptionSPtr volumeDescription_;
    };
}