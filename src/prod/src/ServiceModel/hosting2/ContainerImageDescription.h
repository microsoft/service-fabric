// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ContainerImageDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(ContainerImageDescription);
        DEFAULT_COPY_CONSTRUCTOR(ContainerImageDescription);
        DEFAULT_MOVE_ASSIGNMENT(ContainerImageDescription);
        DEFAULT_MOVE_CONSTRUCTOR(ContainerImageDescription);

    public:
        ContainerImageDescription();
        ContainerImageDescription(std::wstring const & /*imageName*/,
            bool /*UseDefaultRepositoryCredentials*/,
            bool /*UseTokenAuthenticationCredentials*/,
            ServiceModel::RepositoryCredentialsDescription const & repositoryCredentials);

        __declspec(property(get=get_RepositoryCredentials)) ServiceModel::RepositoryCredentialsDescription const & RepositoryCredentials;
        ServiceModel::RepositoryCredentialsDescription const & get_RepositoryCredentials() const { return repositoryCredentials_; }

         __declspec(property(get=get_ImageName)) std::wstring const & ImageName;
        std::wstring const & get_ImageName() const { return imageName_; }

        __declspec(property(get = get_UseDefaultRepositoryCredentials)) bool UseDefaultRepositoryCredentials;
        bool get_UseDefaultRepositoryCredentials() const { return useDefaultRepositoryCredentials_; }

        __declspec(property(get = get_UseTokenAuthenticationCredentials)) bool UseTokenAuthenticationCredentials;
        bool get_UseTokenAuthenticationCredentials() const { return useTokenAuthenticationCredentials_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void SetContainerRepositoryPassword(std::wstring const& password);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_IMAGE_DESCRIPTION & fabricContainerImageDesc) const;

        FABRIC_FIELDS_04(imageName_, repositoryCredentials_, useDefaultRepositoryCredentials_, useTokenAuthenticationCredentials_);

    private:
        std::wstring imageName_;
        bool useDefaultRepositoryCredentials_;
        bool useTokenAuthenticationCredentials_;
        ServiceModel::RepositoryCredentialsDescription repositoryCredentials_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::ContainerImageDescription);
