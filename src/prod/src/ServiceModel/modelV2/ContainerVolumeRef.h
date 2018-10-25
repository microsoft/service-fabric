//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerVolumeRef
        : public ModelType
        {
        public:
            ContainerVolumeRef() = default;

            bool operator==(ContainerVolumeRef const & other) const;
            bool operator!=(ContainerVolumeRef const & other) const;

            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, name_)
                SERIALIZABLE_PROPERTY(L"destinationPath", destinationPath_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::readOnly, readonly_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(destinationPath_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(readonly_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const &) const override;

            FABRIC_FIELDS_03(name_, destinationPath_, readonly_);

        protected:
#ifndef PLATFORM_UNIX

            static std::wregex DestinationPathRegEx;

#endif

            //TODO: add nameformat
            std::wstring name_;

            std::wstring destinationPath_;
            bool readonly_ = false;
        };
    }
}
