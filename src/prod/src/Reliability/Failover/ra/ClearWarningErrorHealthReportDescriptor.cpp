// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;

ClearWarningErrorHealthReportDescriptor::ClearWarningErrorHealthReportDescriptor()
{
}

std::wstring ClearWarningErrorHealthReportDescriptor::GenerateReportDescriptionInternal(HealthContext const & ) const
{
    return L"";
}
