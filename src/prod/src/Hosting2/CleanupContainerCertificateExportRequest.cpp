// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

CleanupContainerCertificateExportRequest::CleanupContainerCertificateExportRequest()
    : certificatePaths_(),
    certificatePasswordPaths_()
{
}

CleanupContainerCertificateExportRequest::CleanupContainerCertificateExportRequest(
    map<std::wstring, std::wstring> certificatePaths,
    map<std::wstring, std::wstring> certificatePasswordPaths)
    : certificatePaths_(certificatePaths),
    certificatePasswordPaths_(certificatePasswordPaths)
{
}

void CleanupContainerCertificateExportRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CleanupContainerCertificateExportRequest { ");
    w.Write("Certificate Paths {");
    for (auto iter = certificatePaths_.begin(); iter != certificatePaths_.end(); iter++)
    {
        w.Write("Certificate Name = {0}", iter->first);
        w.Write("Certificate Path = {0}", iter->second);
    }
    w.Write("}");

    w.Write("Certificate Password Paths {");
    for (auto iter = certificatePasswordPaths_.begin(); iter != certificatePasswordPaths_.end(); iter++)
    {
        w.Write("Certificate Name = {0}", iter->first);
        w.Write("Certificate Password Path = {0}", iter->second);
    }
    w.Write("}");

    w.Write("}");
}
