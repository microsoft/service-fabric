// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ErrorCodeDetails::ErrorCodeDetails()
    : value_(ErrorCodeValue::Success)
    , message_()
{
}

ErrorCodeDetails::ErrorCodeDetails(ErrorCode const& error)
    : value_(error.ReadValue())
    , message_(error.Message)
{
}

ErrorCodeDetails::ErrorCodeDetails(ErrorCode && error)
    : value_(error.ReadValue())
    , message_(error.TakeMessage())
{
}

bool ErrorCodeDetails::HasErrorMessage() const 
{ 
    return !message_.empty(); 
}

ErrorCode ErrorCodeDetails::TakeError()
{
    return ErrorCode(value_, move(message_));
}
