// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ContainerImageDescription : public Serialization::FabricSerializable
    {

    public:
        ContainerImageDescription();
        ContainerImageDescription(
            std::wstring const &,
            ServiceModel::RepositoryCredentialsDescription const &);
        ContainerImageDescription(ContainerImageDescription const & other);
        ContainerImageDescription(ContainerImageDescription && other);

        ContainerImageDescription const & operator = (ContainerImageDescription const & other);
        ContainerImageDescription const & operator = (ContainerImageDescription && other);

        __declspec(property(get=get_RepositoryCredentials)) ServiceModel::RepositoryCredentialsDescription const & RepositoryCredentials;
        ServiceModel::RepositoryCredentialsDescription const & get_RepositoryCredentials() const { return repositoryCredentials_; }

         __declspec(property(get=get_ImageName)) std::wstring const & ImageName;
        std::wstring const & get_ImageName() const { return imageName_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_IMAGE_DESCRIPTION & fabricContainerImageDesc) const;

        FABRIC_FIELDS_02(imageName_, repositoryCredentials_);

    private:
        std::wstring imageName_;
        ServiceModel::RepositoryCredentialsDescription repositoryCredentials_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::ContainerImageDescription);
