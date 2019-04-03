// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace NightWatchTXRService;

std::wstring TestAction::ToString(TestAction::Enum const & val)
{
    switch (val)
    {
    case Enum::Invalid:
        return L"Invalid";
    case Enum::Run:
        return L"Run";
    case Enum::Status:
        return L"Status";
    }
    return L"";
}
