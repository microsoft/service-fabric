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

SecretReferencesDescription::SecretReferencesDescription()
    : ClientServerMessageBody()
    , secretReferences_()
{
}

SecretReferencesDescription::SecretReferencesDescription(std::vector<SecretReference> const & references)
    : ClientServerMessageBody()
    , secretReferences_(references)
{
}

ErrorCode SecretReferencesDescription::FromPublicApi(__in FABRIC_SECRET_REFERENCE_LIST const * referenceList)
{
    return ServiceModel::PublicApiHelper::FromPublicApiList(referenceList, this->secretReferences_);
}

ErrorCode SecretReferencesDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE_LIST & referenceList) const
{
    return ServiceModel::PublicApiHelper::ToPublicApiList<SecretReference, FABRIC_SECRET_REFERENCE, FABRIC_SECRET_REFERENCE_LIST>(heap, this->secretReferences_, referenceList);
}

ErrorCode SecretReferencesDescription::Validate() const
{
    ErrorCode error = ErrorCode::Success();

    for (SecretReference secretRef : this->secretReferences_)
    {
        error = secretRef.Validate();

        if (!error.IsSuccess()) 
        {
            break;
        }
    }

    return error;
}
