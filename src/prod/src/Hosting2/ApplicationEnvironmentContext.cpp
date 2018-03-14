// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

StringLiteral const TraceApplicationEnvironmentContext("AppEnvironmentContext");

ApplicationEnvironmentContext::ApplicationEnvironmentContext(
    ApplicationIdentifier const & applicationId)
    : applicationId_(applicationId),
    principalsContext_(),
    logCollectionPath_(),
    defaultRunAsUserId_()
{
}

ApplicationEnvironmentContext::~ApplicationEnvironmentContext()
{
}

ErrorCode ApplicationEnvironmentContext::SetDefaultRunAsUser(std::wstring const & defaultRunAsUserName)
{
    if (defaultRunAsUserName.empty())
    {
        // No default policy
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto error = TryGetUserId(defaultRunAsUserName, /*out*/ defaultRunAsUserId_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceApplicationEnvironmentContext,
            "Set default runAs policy for user {0}: error {1}",
            defaultRunAsUserName,
            error);
    }

    return error;
}

ErrorCode ApplicationEnvironmentContext::TryGetUserId(wstring const & name, __out wstring & userId) const
{
    if (principalsContext_)
    {
        return principalsContext_->TryGetPrincipalId(name, userId);
    }

    return ErrorCode(ErrorCodeValue::NotFound);
}
