// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceManager;
using namespace Management::CentralSecretService;

SecretResourceMetadata::SecretResourceMetadata(
    wstring const & secretName,
    wstring const & secretVersion)
    : ResourceMetadata(ResourceTypes::Secret)
{
    this->Metadata[MetadataNames::SecretName] = secretName;
    this->Metadata[MetadataNames::SecretVersion] = secretVersion;
}

SecretResourceMetadata::SecretResourceMetadata(
    Secret const & secret)
    : ResourceMetadata(ResourceTypes::Secret)
{
    this->Metadata[MetadataNames::SecretName] = secret.Name;
    this->Metadata[MetadataNames::SecretVersion] = secret.Version;
}