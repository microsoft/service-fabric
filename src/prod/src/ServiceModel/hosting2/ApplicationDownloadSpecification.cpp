// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ApplicationDownloadSpecification::ApplicationDownloadSpecification()
    : applicationId_(),
    applicationVersion_(),
    applicationName_(),
    packageDownloads_()
{
}

ApplicationDownloadSpecification::ApplicationDownloadSpecification(
    ApplicationIdentifier const & applicationId, 
    ApplicationVersion const & applicationVersion, 
    wstring const & applicationName,
    vector<ServicePackageDownloadSpecification> && packageDownloads)
    : applicationId_(applicationId),
    applicationVersion_(applicationVersion),
    applicationName_(applicationName),
    packageDownloads_(move(packageDownloads))
{
}

ApplicationDownloadSpecification::ApplicationDownloadSpecification(ApplicationDownloadSpecification const & other)
    : applicationId_(other.applicationId_),
    applicationVersion_(other.applicationVersion_),
    applicationName_(other.applicationName_),
    packageDownloads_(other.packageDownloads_)
{
}

ApplicationDownloadSpecification::ApplicationDownloadSpecification(ApplicationDownloadSpecification && other)
    : applicationId_(move(other.applicationId_)),
    applicationVersion_(move(other.applicationVersion_)),
    applicationName_(move(other.applicationName_)),
    packageDownloads_(move(other.packageDownloads_))
{
}


ApplicationDownloadSpecification const & ApplicationDownloadSpecification::operator = (ApplicationDownloadSpecification const & other)
{
    if (this != &other)
    {
        this->applicationId_ = other.applicationId_;
        this->applicationVersion_ = other.applicationVersion_;
        this->applicationName_ = other.applicationName_;
        this->packageDownloads_ = other.packageDownloads_;        
    }

    return *this;
}

ApplicationDownloadSpecification const & ApplicationDownloadSpecification::operator = (ApplicationDownloadSpecification && other)
{
    if (this != &other)
    {
        this->applicationId_ = move(other.applicationId_);
        this->applicationVersion_ = move(other.applicationVersion_);
        this->applicationName_ = move(other.applicationName_);
        this->packageDownloads_ = move(other.packageDownloads_);
    }

    return *this;
}

void ApplicationDownloadSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Id={0},Version={1},AppName={2}", applicationId_, applicationVersion_, applicationName_);
   if (packageDownloads_.size() > 0)
   {
       w.Write(",PackageDownloads={");
       for(auto iter = packageDownloads_.cbegin(); iter != packageDownloads_.cend(); ++iter)
       {
           w.Write("{");
           w.Write("{0}", *iter);
           w.Write("}");
       }
       w.Write("}");
   }
}
