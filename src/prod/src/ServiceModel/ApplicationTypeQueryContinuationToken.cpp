// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationTypeQueryContinuationToken::ApplicationTypeQueryContinuationToken()
{
}

ApplicationTypeQueryContinuationToken::ApplicationTypeQueryContinuationToken(
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion)
    : applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
{
}

ApplicationTypeQueryContinuationToken::~ApplicationTypeQueryContinuationToken()
{
}

bool ServiceModel::ApplicationTypeQueryContinuationToken::IsValid() const
{
    // The continuation token is valid iff both are filled out
    return !applicationTypeName_.empty() && !applicationTypeVersion_.empty();
}

void ServiceModel::ApplicationTypeQueryContinuationToken::TakeContinuationTokenComponents(__out std::wstring & typeName, __out std::wstring & typeVersion)
{
    typeName = move(applicationTypeName_);
    typeVersion = move(applicationTypeVersion_);
}




