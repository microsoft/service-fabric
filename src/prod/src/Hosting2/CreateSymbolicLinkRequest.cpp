// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

CreateSymbolicLinkRequest::CreateSymbolicLinkRequest() 
    : symbolicLinks_(),
    flags_(0)
{
}

CreateSymbolicLinkRequest::CreateSymbolicLinkRequest(
    vector<ArrayPair<wstring, wstring>> const & symbolicLinks,
    DWORD flags)
    : symbolicLinks_(symbolicLinks),
    flags_(flags)
{
}

void CreateSymbolicLinkRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CreateSymbolicLinkRequest { ");
    w.Write("SymbolicLinks {");

    for(auto iter = symbolicLinks_.begin(); iter != symbolicLinks_.end(); iter++)
    {
        w.Write("SymbolicLinks {0}, TargetName {1}", iter->key, iter->value);
    }

    w.Write("}");
    w.Write("Flags = {0}", flags_);
    w.Write("}");
}
