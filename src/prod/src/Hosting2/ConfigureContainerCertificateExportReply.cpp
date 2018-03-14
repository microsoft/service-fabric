// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureContainerCertificateExportReply::ConfigureContainerCertificateExportReply()
    : error_(),
    errorMessage_(),
    certificatePaths_(),
    certificatePasswordPaths_()
{
}

ConfigureContainerCertificateExportReply::ConfigureContainerCertificateExportReply(
    Common::ErrorCode const & error,
    wstring const & errorMessage,
    map<std::wstring, std::wstring> certificatePaths,
    map<std::wstring, std::wstring> certificatePasswordPaths)
    : error_(error),
    errorMessage_(errorMessage),
    certificatePaths_(certificatePaths),
    certificatePasswordPaths_(certificatePasswordPaths)
{
}

void ConfigureContainerCertificateExportReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureContainerCertificateExportReply { ");
    w.Write("Error = {0}", error_);
    w.Write("Error message = {0}", errorMessage_);
    w.Write("Certificate PFX Paths {");
    for (auto iter = certificatePaths_.begin(); iter != certificatePaths_.end(); iter++)
    {
        w.Write("Certificate Name = {0}", iter->first);
        w.Write("Certificate PFX Path = {0}", iter->second);
    }
    w.Write("}");

    w.Write("Certificate Password Paths {");
    for (auto iter = certificatePaths_.begin(); iter != certificatePaths_.end(); iter++)
    {
        w.Write("Certificate Name = {0}", iter->first);
        w.Write("Certificate Password Path = {0}", iter->second);
    }
    w.Write("}");

    w.Write("}");
}
