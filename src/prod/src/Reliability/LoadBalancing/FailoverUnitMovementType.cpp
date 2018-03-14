// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FailoverUnitMovementType.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace FailoverUnitMovementType
        {
            void FailoverUnitMovementType::WriteToTextWriter(TextWriter & w, FailoverUnitMovementType::Enum const & val)
            {
                w << ToString(val);
            }

            ENUM_STRUCTURED_TRACE(FailoverUnitMovementType, SwapPrimarySecondary, LastValidEnum);
        }
    }
}

wstring FailoverUnitMovementType::ToString(FailoverUnitMovementType::Enum failoverUnitMovementType)
{
    switch (failoverUnitMovementType)
    {
    case FailoverUnitMovementType::SwapPrimarySecondary:
        return L"SwapPrimarySecondary";
    case FailoverUnitMovementType::MoveSecondary:
        return L"MoveSecondary";
    case FailoverUnitMovementType::MovePrimary:
        return L"MovePrimary";
    case FailoverUnitMovementType::AddPrimary:
        return L"AddPrimary";
    case FailoverUnitMovementType::AddSecondary:
        return L"AddSecondary";
    case FailoverUnitMovementType::PromoteSecondary:
        return L"PromoteSecondary";
    case FailoverUnitMovementType::Void:
        return L"Void";
    case FailoverUnitMovementType::Drop:
        return L"Drop";
    default:
        Common::Assert::CodingError("Unknown FailoverUnit Movement Type {0}", static_cast<int>(failoverUnitMovementType));
    }
}
