// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    double RoundTo(double number, int numberOfDecimalPlacesToPreserve)
    {
        double roundfactor = pow(10, numberOfDecimalPlacesToPreserve);
        return floor(number * roundfactor + 0.5) / roundfactor;
    }
}
