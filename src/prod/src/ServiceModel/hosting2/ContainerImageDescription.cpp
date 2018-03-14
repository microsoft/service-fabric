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
    repositoryCredentials_()
{
}

ContainerImageDescription::ContainerImageDescription(
    wstring const & imageName,
    RepositoryCredentialsDescription const & repositoryCredentials) 
    : imageName_(imageName),
    repositoryCredentials_(repositoryCredentials)
{
}

ContainerImageDescription::ContainerImageDescription(
    ContainerImageDescription const & other) 
    : imageName_(other.imageName_),
    repositoryCredentials_(other.repositoryCredentials_)
{
}

ContainerImageDescription::ContainerImageDescription(ContainerImageDescription && other) 
    : imageName_(move(other.imageName_)),
    repositoryCredentials_(move(other.repositoryCredentials_))
{
}

ContainerImageDescription const & ContainerImageDescription::operator = (ContainerImageDescription const & other)
{
    if (this != &other)
    {
        this->imageName_ = other.imageName_;
        this->repositoryCredentials_ = other.repositoryCredentials_;
    }

    return *this;
}

ContainerImageDescription const & ContainerImageDescription::operator = (ContainerImageDescription && other)
{
    if (this != &other)
    {
        this->imageName_ = move(other.imageName_);
        this->repositoryCredentials_ = move(other.repositoryCredentials_);
    }

    return *this;
}

void ContainerImageDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerImageDescription { ");
    w.Write("imageName = {0}, ", imageName_);
    w.Write("}");
}

ErrorCode ContainerImageDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_IMAGE_DESCRIPTION & fabricContainerImageDesc) const
{
    fabricContainerImageDesc.ImageName = heap.AddString(this->ImageName);

    auto  repoCredential = heap.AddItem<FABRIC_REPOSITORY_CREDENTIAL_DESCRIPTION>();
    auto error = this->RepositoryCredentials.ToPublicApi(heap, *repoCredential);
    if (!error.IsSuccess())
    {
        return error;
    }
    fabricContainerImageDesc.RepositoryCredential = repoCredential.GetRawPointer();

    fabricContainerImageDesc.Reserved = nullptr;

    return ErrorCode(ErrorCodeValue::Success);
}

