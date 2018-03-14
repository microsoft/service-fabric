// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureContainerCertificateExportRequest::ConfigureContainerCertificateExportRequest()
: certificateRef_(),
  workDirectoryPath_()
{
}

ConfigureContainerCertificateExportRequest::ConfigureContainerCertificateExportRequest(
    std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
    wstring const & workDirectoryPath)
    : certificateRef_(certificateRef),
    workDirectoryPath_(workDirectoryPath)
{
}

void ConfigureContainerCertificateExportRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureContainerCertificateExportRequest { ");
    w.Write("ContainerCertificateDescription {");
    for (auto iter = certificateRef_.begin(); iter != certificateRef_.end(); ++iter)
    {
        w.Write("CodePackageName = {0}", iter->first);
        w.Write("Certificates {");
        vector<ContainerCertificateDescription> certDesc = iter->second;		
        for (auto it = certDesc.begin(); it != certDesc.end(); ++it)
        {
            w.Write("Name = {0}", it->Name);
            w.Write("X509Store = {0}", it->X509StoreName);
            w.Write("X509FindValue = {0}", it->X509FindValue);
        }
        w.Write("}");
    }
    w.Write("}");
    w.Write("WorkDirectoryPath = {0}", workDirectoryPath_);
    w.Write("}");
}
