// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

void ApplicationHostRegistrationStatus::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

wstring ApplicationHostRegistrationStatus::ToString(Enum const & val)
{
    switch(val)
    {
    case ApplicationHostRegistrationStatus::NotRegistered:
        return L"NotRegistered";
    case ApplicationHostRegistrationStatus::InProgress:
        return L"InProgress";
    case ApplicationHostRegistrationStatus::InProgress_Monitored:
        return L"InProgress_Monitored";
    case ApplicationHostRegistrationStatus::Completed:
        return L"Completed";
    case ApplicationHostRegistrationStatus::Timedout:
        return L"Timedout";
    default:
        Assert::CodingError("Unknown ApplicationHostRegistrationStatus value {0}", (int)val);
    }
}
