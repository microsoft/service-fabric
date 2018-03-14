// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DiagnosticSubspace.h"

using namespace std;
using namespace Reliability::LoadBalancingComponent;

void DiagnosticSubspace::RefreshSubspacesDiagnostics(std::vector<DiagnosticSubspace> & subspaces)
{
    for (auto it = subspaces.begin(); it != subspaces.end(); ++it)
    {
        if (it->Count == std::numeric_limits<size_t>::max())
        {
            continue;
        }
        else
        {
            it->Count = 0;
            it->NodeDetail = L"";
        }
    }
}
