// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void PartitionSchemeDescription::WriteToTextWriter(__in TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case PartitionSchemeDescription::Singleton:
        w << L"Singleton";
        return;
    case PartitionSchemeDescription::UniformInt64Range:
        w << L"UniformInt64";
        return;
    case PartitionSchemeDescription::Named:
        w << L"Named";
        return;
    default:
        Assert::CodingError("Unknown PartitionSchemeDescription value {0}", static_cast<int>(val));
    }
}
