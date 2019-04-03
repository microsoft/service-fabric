// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::CentralSecretService;

SecretsDescription::SecretsDescription()
    : ClientServerMessageBody()
    , secrets_()
{
}

SecretsDescription::SecretsDescription(std::vector<Secret> const & secrets)
    : ClientServerMessageBody()
    , secrets_(secrets)
{
}

ErrorCode SecretsDescription::FromPublicApi(__in FABRIC_SECRET_LIST const * secretList)
{
    return ServiceModel::PublicApiHelper::FromPublicApiList(secretList, this->secrets_);
}

ErrorCode SecretsDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SECRET_LIST & secretList) const
{
    return ServiceModel::PublicApiHelper::ToPublicApiList<Secret, FABRIC_SECRET, FABRIC_SECRET_LIST>(heap, this->secrets_, secretList);
}

ErrorCode SecretsDescription::Validate() const
{
    ErrorCode error = ErrorCode::Success();

    for (Secret secret : this->secrets_)
    {
        error = secret.Validate();

        if (!error.IsSuccess())
        {
            break;
        }
    }

    return error;
}

