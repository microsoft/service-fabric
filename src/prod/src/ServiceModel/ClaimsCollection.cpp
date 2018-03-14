// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ClaimsCollection::ClaimsCollection()
    : claimsList_()
{
}

ClaimsCollection::ClaimsCollection(vector<Claim> && claimsList)
    : claimsList_(move(claimsList))
{
}

void ClaimsCollection::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ClaimsCollection { ");
    for(auto iter = this->claimsList_.begin(); iter != this->claimsList_.end(); ++iter)
    {
        w.Write("Claim = {0}", *iter);
    }
    w.Write("}");
}

