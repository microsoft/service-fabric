// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

DeleteApplicationFoldersReply::DeleteApplicationFoldersReply() 
    : error_()
{
}

DeleteApplicationFoldersReply::DeleteApplicationFoldersReply(
    ErrorCode const & error,
    vector<wstring> const & deletedAppFolders)
    : error_(error),
    deletedAppFolders_(deletedAppFolders)
{
}

void DeleteApplicationFoldersReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeleteApplicationFoldersReply { ");
    w.Write("Error = {0}", error_);
    w.Write("Deleted AppFolders");
    for (auto iter = deletedAppFolders_.begin(); iter != deletedAppFolders_.end(); iter++)
    {
        w.Write("Application Folder = {0}", *iter);
    }
    w.Write("}");
}
