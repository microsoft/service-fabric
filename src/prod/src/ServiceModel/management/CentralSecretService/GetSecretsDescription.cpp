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

GetSecretsDescription::GetSecretsDescription()
    : SecretReferencesDescription(),
    includeValue_() {}

GetSecretsDescription::GetSecretsDescription(std::vector<SecretReference> const & secretReferences, bool includeValue)
    : SecretReferencesDescription(move(secretReferences))
    , includeValue_(includeValue) {}

GetSecretsDescription::~GetSecretsDescription()
{
}

Common::ErrorCode GetSecretsDescription::FromPublicApi(__in FABRIC_SECRET_REFERENCE_LIST const * secretReferenceList, __in BOOLEAN includeValue)
{
    this->includeValue_ = includeValue == 0 ? false : true;
    return SecretReferencesDescription::FromPublicApi(secretReferenceList);
}

Common::ErrorCode GetSecretsDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE_LIST & secretReferenceList, __out BOOLEAN & includeValue) const
{
    includeValue = this->includeValue_ ? 1 : 0;
    return SecretReferencesDescription::ToPublicApi(heap, secretReferenceList);
}