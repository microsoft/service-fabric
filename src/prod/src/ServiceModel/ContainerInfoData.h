// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ContainerInfoData : public Common::IFabricJsonSerializable
    {
        public:
            ContainerInfoData() = default;
            ContainerInfoData(std::wstring&& content)
                :content_(move(content))
            {}

            __declspec(property(get = get_Content)) std::wstring const& Content;
            std::wstring const& get_Content() const { return content_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Content, content_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring content_;
    };
}
