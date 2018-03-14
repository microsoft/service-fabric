// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::TokenValidationService;
using namespace ServiceModel;

TokenValidationMessage::TokenValidationMessage() 
    : claims_()  
    , tokenExpiryTime_()
    , error_()
{
}

TokenValidationMessage::TokenValidationMessage(
    ClaimsCollection const & claims,
    TimeSpan const & tokenExpiryTime,
    ErrorCode const & error)
    : claims_(claims)
    , tokenExpiryTime_(tokenExpiryTime)
    , error_(error)
{
}

void TokenValidationMessage::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TokenValidationReply { ");
    w.Write("Error = {0}", Error);
    w.Write("ClaimsResult = {0}", ClaimsResult);
    w.Write("TokenExpiryTime = {0}", tokenExpiryTime_);
    w.Write("}");
}
