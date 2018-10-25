//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataVolume : public Store::StoreData
        {
        public:
            StoreDataVolume() = default;

            StoreDataVolume(std::wstring const &);

            StoreDataVolume(VolumeDescriptionSPtr const &);

            StoreDataVolume(StoreDataVolume &&) = default;
            StoreDataVolume & operator=(StoreDataVolume &&) = default;

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            __declspec(property(get=get_VolumeName)) std::wstring const & VolumeName;
            std::wstring const & get_VolumeName() const { return volumeName_; }

            __declspec(property(get=get_VolumeDescription)) Management::ClusterManager::VolumeDescriptionSPtr const & VolumeDescription;
            Management::ClusterManager::VolumeDescriptionSPtr const & get_VolumeDescription() const { return volumeDescription_; }

            FABRIC_FIELDS_02(volumeName_, volumeDescription_);

        protected:
            virtual std::wstring ConstructKey() const override;
        private:
            std::wstring volumeName_;
            Management::ClusterManager::VolumeDescriptionSPtr volumeDescription_;
        };
    }
}
