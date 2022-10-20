// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace NightWatchTXRService;

std::wstring TestStatus::ToString(TestStatus::Enum const & val)
{
    switch (val)
    {
    case Enum::Invalid:
        return L"Invalid";
    case Enum::Running:
        return L"Running";
    case Enum::Finished:
        return L"Finished";
    case Enum::NotStarted:
        return L"NotStarted";
    }
    return L"";
}
