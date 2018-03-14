// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;

ServiceModelTypeName::ServiceModelTypeName()
    : TypeSafeString()
{
}

ServiceModelTypeName::ServiceModelTypeName(wstring const & value)
    : TypeSafeString(value)
{
}
