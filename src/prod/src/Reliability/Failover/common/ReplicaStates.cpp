// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability::ReplicaStates;

void Reliability::ReplicaStates::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case StandBy: 
            w << L"SB"; return;
        case InBuild: 
            w << L"IB"; return;
        case Ready: 
            w << L"RD"; return;
        case Dropped: 
            w << L"DD"; return;
        default: 
            w << "ReplicaStates(" << static_cast<uint>(val) << ')';
    }
}

std::wstring Reliability::ReplicaStates::ToString(Enum const & val)
{
    switch (val)
    {
    case StandBy:
        return L"SB";
    case InBuild:
        return L"IB";
    case Ready:
        return L"RD";
    case Dropped:
        return L"DD";
    default:
        return L"ReplicaStates(" + StringUtility::ToWString(static_cast<uint>(val)) + L")";
    }
}
