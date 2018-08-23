// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

namespace Hosting2
{
    namespace ApplicationHostCodePackageOperationType
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << "Invalid"; return;
            case Activate: w << "Activate"; return;
            case Deactivate: w << "Deactivate"; return;
            case Abort: w << "Abort"; return;
            case ActivateAll: w << "ActivateAll"; return;
            case DeactivateAll: w << "DeactivateAll"; return;
            case AbortAll: w << "AbortAll"; return;
            default: w << "ApplicationHostCodePackageOperationType(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

        ENUM_STRUCTURED_TRACE(ApplicationHostCodePackageOperationType, FirstValidEnum, LastValidEnum)
    }
}

ApplicationHostCodePackageOperationRequest::ApplicationHostCodePackageOperationRequest()
    : operationType_(ApplicationHostCodePackageOperationType::Invalid)
    , appHostContext_()
    , codePackageContext_()
    , codePackageNames_()
    , environmentBlock_()
    , timeoutTicks_(0)
{
}

ApplicationHostCodePackageOperationRequest::ApplicationHostCodePackageOperationRequest(
    ApplicationHostCodePackageOperationType::Enum operationType,
    ApplicationHostContext const & appHostContext,
    CodePackageContext const & codePackageContext,
    vector<wstring> const & codePackageNames,
    EnvironmentMap const & environment,
    int64 timeoutTicks)
    : operationType_(operationType)
    , appHostContext_(appHostContext)
    , codePackageContext_(codePackageContext)
    , codePackageNames_(codePackageNames)
    , timeoutTicks_(timeoutTicks)
{
    Environment::ToEnvironmentBlock(environment, environmentBlock_);
}

void ApplicationHostCodePackageOperationRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageOperationRequest { ");
    w.Write("OperationType = {0}, ", OperationType);
    w.Write("ApplicationHostContext = {0}, ", HostContext);
    w.Write("CodePackageContext = {0}, ", CodeContext);
    w.Write("TimeoutInTicks = {0}, ", TimeoutInTicks);

    w.Write(", CodePackages = { ");

    for (auto const & cpName : CodePackageNames)
    {
        w.Write("{0}, ", cpName);
    }

    w.Write("}}");
}
