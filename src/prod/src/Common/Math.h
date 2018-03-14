// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //this function is to round double numbers to the given number of decimal places
    //numberOfDecimalPlacesToPreserve must be larger than the actual decimal places in the input number (function cannot be used to add more decimal places to the input)
    //number * 10^decimalPlaces should no get bigger than system's int maxvalue, or the function overflows
    double RoundTo(double number, int numberOfDecimalPlacesToPreserve);
}
