// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"
#include "ReplicaRole.h"

using namespace Common;
using namespace Reliability::LoadBalancingComponent::ReplicaRole;

void Reliability::LoadBalancingComponent::ReplicaRole::WriteToTextWriter(Common::TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case None:
            w << "None";
            return;
        case Primary:
            w << "Primary";
            return;
        case Secondary:
            w << "Secondary";
            return;
        case StandBy:
            w << "StandBy";
            return;
        case Dropped:
            w << "Dropped";
            return;
        default:
            Assert::CodingError("Unknown Replica Role Type {0}", static_cast<int>(val));
    }
}

std::wstring Reliability::LoadBalancingComponent::ReplicaRole::ToString(Enum const & val)
{
    switch (val)
    {
    case None:
        return L"None";
    case Primary:
        return L"Primary";
    case Secondary:
        return L"Secondary";
    case StandBy:
        return L"StandBy";
    case Dropped:
        return L"Dropped";
    default:
        Assert::CodingError("Unknown Replica Role Type {0}", static_cast<int>(val));
    }
}
