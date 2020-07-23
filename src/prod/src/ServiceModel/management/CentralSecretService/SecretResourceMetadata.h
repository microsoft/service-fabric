// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        namespace MetadataNames
        {
            static Common::GlobalWString SecretName = Common::make_global<std::wstring>(L"SecretName");
            static Common::GlobalWString SecretVersion = Common::make_global<std::wstring>(L"SecretVersion");
        }

        class SecretResourceMetadata : public Management::ResourceManager::ResourceMetadata
        {
        public:
            SecretResourceMetadata(std::wstring const & secretName, std::wstring const & secretVersion);

            SecretResourceMetadata(Secret const & secret);

            __declspec (property(get = get_SecretName)) std::wstring const & SecretName;
            std::wstring const & get_SecretName() { return this->Metadata[MetadataNames::SecretName]; }

            __declspec (property(get = get_SecretVersion)) std::wstring const & SecretVersion;
            std::wstring const & get_SecretVersion() { return this->Metadata[MetadataNames::SecretVersion]; }
        };
    }
}