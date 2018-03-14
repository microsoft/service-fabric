// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;

void PartitionKind::WriteToTextWriter(TextWriter & w, Enum const& value)
{
    switch (value)
    {
    case Invalid:
        w << L"Invalid";
        return;
    case Singleton:
        w << L"Singleton";
        return;
    case UniformInt64:
        w << L"UniformInt64";
        return;
    case Named:
        w << L"Named";
        return;
    default:
        w << "PartitionKind(" << static_cast<uint>(value) << ')';
        return;
    }
}

PartitionKind::Enum PartitionKind::FromPublicApi(FABRIC_SERVICE_PARTITION_KIND const publicKind)
{
    switch (publicKind)
    {
    case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
        return Singleton;
    case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
        return UniformInt64;
    case FABRIC_SERVICE_PARTITION_KIND_NAMED:
        return Named;
    default:
        return Invalid;
    }
}
