// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ResourceDescription
            : public ModelType
        {
        public:
            ResourceDescription() = default;

            ResourceDescription(std::wstring const &invalidNameCharacters)
                : invalidNameCharacters_(invalidNameCharacters)
                , Name()
            {
            }

            ResourceDescription(std::wstring const &invalidNameCharacters, std::wstring const &name)
                : invalidNameCharacters_(invalidNameCharacters)
                , Name(name)
            {

            }

            bool operator==(ResourceDescription const & other) const
            {
                return this->Name == other.Name;
            }

            bool operator!=(ResourceDescription const & other) const
            {
                return !(*this == other);
            }

            virtual Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(Constants::nameCamelCase, Name)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Name)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_01(Name)

        public:
            std::wstring Name;

        private:
            std::wstring invalidNameCharacters_;
        };
    }
}
