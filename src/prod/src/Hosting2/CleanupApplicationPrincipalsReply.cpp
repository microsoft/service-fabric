// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

CleanupApplicationPrincipalsReply::CleanupApplicationPrincipalsReply() 
    : error_()
{
}

CleanupApplicationPrincipalsReply::CleanupApplicationPrincipalsReply(
    ErrorCode const & error,
    vector<wstring> const & deletedAppIds)
    : error_(error),
    deletedAppIds_(deletedAppIds)
{
}

void CleanupApplicationPrincipalsReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CleanupApplicationPrincipalsReply { ");
    w.Write("Error = {0}", error_);
    w.Write("Deleted Principal AppIds");
    for (auto iter = deletedAppIds_.begin(); iter != deletedAppIds_.end(); iter++)
    {
        w.Write("Application Id = {0}", *iter);
    }
    w.Write("}");
}
