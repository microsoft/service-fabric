// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerImageDescription::ContainerImageDescription() 
    : imageName_(),
    useDefaultRepositoryCredentials_(),
    useTokenAuthenticationCredentials_(),
    repositoryCredentials_()
{
}

ContainerImageDescription::ContainerImageDescription(
    wstring const & imageName,
    bool useDefaultRepositoryCredentials,
    bool useTokenAuthenticationCredentials,
    RepositoryCredentialsDescription const & repositoryCredentials) 
    : imageName_(imageName),
    useDefaultRepositoryCredentials_(useDefaultRepositoryCredentials),
    useTokenAuthenticationCredentials_(useTokenAuthenticationCredentials),
    repositoryCredentials_(repositoryCredentials)
{
}

void ContainerImageDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerImageDescription { ");
    w.Write("imageName = {0}, ", imageName_);
    w.Write("useDefaultRepositoryCredentials_ = {0}, ", useDefaultRepositoryCredentials_);
    w.Write("useTokenAuthenticationCredentials_ = {0}, ", useTokenAuthenticationCredentials_);
    w.Write("}");
}

ErrorCode ContainerImageDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_IMAGE_DESCRIPTION & fabricContainerImageDesc) const
{
    fabricContainerImageDesc.ImageName = heap.AddString(this->ImageName);

    auto repoCredential = heap.AddItem<FABRIC_REPOSITORY_CREDENTIAL_DESCRIPTION>();
    auto error = this->RepositoryCredentials.ToPublicApi(heap, *repoCredential);
    if (!error.IsSuccess())
    {
        return error;
    }

    fabricContainerImageDesc.RepositoryCredential = repoCredential.GetRawPointer();

    auto fabricContainerImageDescEx1 = heap.AddItem<FABRIC_CONTAINER_IMAGE_DESCRIPTION_EX1>();
    fabricContainerImageDescEx1->UseDefaultRepositoryCredentials = this->UseDefaultRepositoryCredentials;
    fabricContainerImageDesc.Reserved = fabricContainerImageDescEx1.GetRawPointer();

    auto fabricContainerImageDescEx2 = heap.AddItem<FABRIC_CONTAINER_IMAGE_DESCRIPTION_EX2>();
    fabricContainerImageDescEx2->UseTokenAuthenticationCredentials = this->UseTokenAuthenticationCredentials;
    fabricContainerImageDescEx2->Reserved = nullptr;

    fabricContainerImageDescEx1->Reserved = fabricContainerImageDescEx2.GetRawPointer();

    return ErrorCode(ErrorCodeValue::Success);
}

void ContainerImageDescription::SetContainerRepositoryPassword(std::wstring const& password)
{
    repositoryCredentials_.Password = password;
}