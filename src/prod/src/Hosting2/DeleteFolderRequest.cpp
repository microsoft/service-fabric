// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

DeleteFolderRequest::DeleteFolderRequest() 
    : nodeId_(),
    appFolders_()
{
}

DeleteFolderRequest::DeleteFolderRequest(
    wstring const & nodeId,
    vector<wstring> const & appInstanceFolders)
    : nodeId_(nodeId),
    appFolders_(appInstanceFolders)
{
}

void DeleteFolderRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeleteFolderRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    for (auto iter = appFolders_.begin(); iter != appFolders_.end(); ++iter)
    {
        w.Write("ApplicationFolder = {0}", (*iter));
    }
    w.Write("}");
}
