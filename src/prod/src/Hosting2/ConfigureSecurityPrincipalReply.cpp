// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureSecurityPrincipalReply::ConfigureSecurityPrincipalReply() 
    : error_(ErrorCodeValue::Success),
    principalsInformation_()
{
}

ConfigureSecurityPrincipalReply::ConfigureSecurityPrincipalReply(
    vector<SecurityPrincipalInformationSPtr> const & principalsInformation,
    ErrorCode const & error)
    : error_(error),
    principalsInformation_()
{
    if(error_.IsSuccess())
    {
        for(auto iter=principalsInformation.begin(); iter != principalsInformation.end(); ++iter)
        {
            principalsInformation_.push_back(*(*iter));
        }
    }
}

void ConfigureSecurityPrincipalReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureSecurityPrincipalReply { ");
    w.Write("ErrorCode = {0}", error_);
    w.Write("User : Sid");
    for(auto iter = principalsInformation_.begin(); iter != principalsInformation_.end(); ++iter)
    {
        w.Write(" PrincipalInformation = {0}", *iter);
    }
    w.Write("}");
}
