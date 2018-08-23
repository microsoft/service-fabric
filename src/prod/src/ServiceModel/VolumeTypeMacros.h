// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SERVICE_FABRIC_VOLUME_DISK_TYPE(BaseClassName, ClassName)                                               \
    class ClassName : public BaseClassName                                                                      \
    {                                                                                                           \
        DENY_COPY(ClassName)                                                                                    \
        public:                                                                                                 \
            ClassName()=default;                                                                                \
            virtual ~ClassName()=default;                                                                       \
            bool operator==(ClassName const & other) const                                                      \
            {                                                                                                   \
                return (sizeDisk_ == other.sizeDisk_);                                                          \
            }                                                                                                   \
            bool operator!=(ClassName const & other) const                                                      \
            {                                                                                                   \
                return !(*this == other);                                                                       \
            }                                                                                                   \
            __declspec(property(get = get_SizeDisk)) std::wstring const & SizeDisk;                             \
            std::wstring const & get_SizeDisk() const { return sizeDisk_; }                                     \
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const & fOpt) const override     \
            {                                                                                                   \
                BaseClassName::WriteTo(w, fOpt);                                                                \
                w.Write("SizeDisk {0}]", sizeDisk_);                                                            \
            }                                                                                                   \
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()                                                                \
                SERIALIZABLE_PROPERTY_CHAIN();                                                                  \
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::sizeDisk, sizeDisk_)                             \
            END_JSON_SERIALIZABLE_PROPERTIES()                                                                  \
            BEGIN_DYNAMIC_SIZE_ESTIMATION()                                                                     \
                DYNAMIC_SIZE_ESTIMATION_CHAIN();                                                                \
                DYNAMIC_SIZE_ESTIMATION_MEMBER(sizeDisk_)                                                       \
            END_DYNAMIC_SIZE_ESTIMATION()                                                                       \
            FABRIC_FIELDS_01(                                                                                   \
                sizeDisk_)                                                                                      \
        protected:                                                                                              \
            Common::ErrorCode Validate(                                                                         \
                std::wstring const & volumeName,                                                                \
                int maxVolumeNameLength) const                                                                  \
            {                                                                                                   \
                Common::ErrorCode error = BaseClassName::Validate();                                            \
                if (!error.IsSuccess())                                                                         \
                {                                                                                               \
                    return error;                                                                               \
                }                                                                                               \
                if (volumeName.length() > maxVolumeNameLength)                                                  \
                {                                                                                               \
                    return Common::ErrorCode(                                                                   \
                        Common::ErrorCodeValue::InvalidArgument,                                                \
                        Common::wformatString(                                                                  \
                            GET_CM_RC(VolumeDiskVolumeNameTooLong),                                             \
                            maxVolumeNameLength));                                                              \
                }                                                                                               \
                return error;                                                                                   \
            }                                                                                                   \
        private:                                                                                                \
            std::wstring sizeDisk_;                                                                             \
    };                                                                                                          \


