// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationDescription");

ApplicationDescription::ApplicationDescription() 
    : applicationName_()
    , applicationTypeName_()
    , applicationTypeVersion_()
    , applicationParameters_()
{ 
}

ApplicationDescription::ApplicationDescription(
    NamingUri const & appName,
    std::wstring const & applicationTypeName,
    std::wstring const & applicationTypeVersion,
    std::map<std::wstring, std::wstring> const & applicationParameters)
    : applicationName_(appName)
    , applicationTypeName_(applicationTypeName)
    , applicationTypeVersion_(applicationTypeVersion)
    , applicationParameters_(applicationParameters)
{
}

ApplicationDescription::ApplicationDescription(ApplicationDescription && other) 
    : applicationName_(move(other.applicationName_))
    , applicationTypeName_(move(other.applicationTypeName_))
    , applicationTypeVersion_(move(other.applicationTypeVersion_))
    , applicationParameters_(move(other.applicationParameters_))
{ 
}

Common::ErrorCode ApplicationDescription::FromPublicApi(FABRIC_APPLICATION_DESCRIPTION const & publicDescription)
{
    if (!NamingUri::TryParse(publicDescription.ApplicationName, applicationName_))
    {
        Trace.WriteWarning(TraceComponent, "Cannot parse '{0}' as Naming Uri", wstring(publicDescription.ApplicationName));

        return ErrorCodeValue::InvalidNameUri;
    }

    applicationTypeName_ = wstring(publicDescription.ApplicationTypeName);
    applicationTypeVersion_ = wstring(publicDescription.ApplicationTypeVersion);

    if (publicDescription.ApplicationParameters != NULL)
    {
        for (ULONG ix = 0; ix < publicDescription.ApplicationParameters->Count; ++ix)
        {
            FABRIC_APPLICATION_PARAMETER parameter = publicDescription.ApplicationParameters->Items[ix];

            applicationParameters_[wstring(parameter.Name)] = wstring(parameter.Value);
        }
    }

    return ErrorCodeValue::Success;
}

void ApplicationDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_DESCRIPTION & publicDescription) const
{
    publicDescription.ApplicationName = heap.AddString(applicationName_.ToString());
    publicDescription.ApplicationTypeName = heap.AddString(applicationTypeName_);
    publicDescription.ApplicationTypeVersion = heap.AddString(applicationTypeVersion_);

    if (!applicationParameters_.empty())
    {
        auto parameterCollection = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
        publicDescription.ApplicationParameters = parameterCollection.GetRawPointer();
        parameterCollection->Count = static_cast<ULONG>(applicationParameters_.size());

        auto parameters = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(parameterCollection->Count);
        parameterCollection->Items = &parameters[0];

        int index = 0;
        for (auto iter = applicationParameters_.begin(); iter != applicationParameters_.end(); ++iter, ++index)
        {
            parameters[index].Name = heap.AddString(iter->first);
            parameters[index].Value = heap.AddString(iter->second);
        }
    }
    else
    {
        publicDescription.ApplicationParameters = NULL;
    }
}

bool ApplicationDescription::operator == (ApplicationDescription const & other) const
{
    bool isMatch = applicationName_ == other.applicationName_
        && applicationTypeName_ == other.applicationTypeName_
        && applicationTypeVersion_ == other.applicationTypeVersion_;

    if (isMatch)
    {
        isMatch = (applicationParameters_.size() == other.applicationParameters_.size());
    }

    if (isMatch)
    {
        for (auto iter = applicationParameters_.begin(); iter != applicationParameters_.end(); ++iter)
        {
            auto thisName = iter->first;
            auto thisValue = iter->second;

            auto iter2 = other.applicationParameters_.find(thisName);
            if (iter2 == other.applicationParameters_.end())
            {
                isMatch = false;
                break;
            }
            else if (thisValue != iter2->second)
            {
                isMatch = false;
                break;
            }
        }
    }

    return isMatch;
}

bool ApplicationDescription::operator != (ApplicationDescription const & other) const
{
    return !(*this == other);
}

void ApplicationDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0}, {1}, {2}, Parameters = {3}",
        applicationName_,
        applicationTypeName_,
        applicationTypeVersion_,
        applicationParameters_);
}
