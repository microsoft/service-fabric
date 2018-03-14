// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ConfigreSharedFolderACLRequest::ConfigreSharedFolderACLRequest() 
    : sharedFolders_(),
    ticks_()
{
}

ConfigreSharedFolderACLRequest::ConfigreSharedFolderACLRequest(
    vector<wstring> const & sharedFolders,
    int64 timeoutTicks)
    : sharedFolders_(sharedFolders),
    ticks_(timeoutTicks)
{
}

void ConfigreSharedFolderACLRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigreSharedFolderACLRequest { ");
    w.Write("SharedFolders {");

    for (auto iter = sharedFolders_.begin(); iter != sharedFolders_.end(); iter++)
    {
        w.Write("SharedFolder {0}", *iter);
    }

    w.Write("}");
    w.Write("TimeOut ticks {0}", ticks_);
    w.Write("}");
}
