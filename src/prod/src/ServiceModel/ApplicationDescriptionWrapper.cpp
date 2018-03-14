// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationDescriptionWrapper::ApplicationDescriptionWrapper()
    : applicationName_()
    , applicationTypeName_()
    , applicationTypeVersion_()
    , parameterList_()
    , applicationCapacity_()
{
}

ApplicationDescriptionWrapper::ApplicationDescriptionWrapper(
    std::wstring const &appName,
    std::wstring const &appTypeName,
    std::wstring const &appTypeVersion,
    std::map<std::wstring, std::wstring> const &parameterList)
    : applicationName_(appName)
    , applicationTypeName_(appTypeName)
    , applicationTypeVersion_(appTypeVersion)
    , parameterList_(parameterList)
    , applicationCapacity_()
{
}

ApplicationDescriptionWrapper::ApplicationDescriptionWrapper(
    std::wstring const &appName,
    std::wstring const &appTypeName,
    std::wstring const &appTypeVersion,
    std::map<std::wstring, std::wstring> const &parameterList,
    Reliability::ApplicationCapacityDescription const & applicationCapacity)
    : applicationName_(appName)
    , applicationTypeName_(appTypeName)
    , applicationTypeVersion_(appTypeVersion)
    , parameterList_(parameterList)
    , applicationCapacity_(applicationCapacity)
{
}

Common::ErrorCode ApplicationDescriptionWrapper::FromPublicApi(__in FABRIC_APPLICATION_DESCRIPTION const &appDesc)
{
    auto error = StringUtility::LpcwstrToWstring2(appDesc.ApplicationName, false, applicationName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(appDesc.ApplicationTypeName, false, applicationTypeName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(appDesc.ApplicationTypeVersion, false, applicationTypeVersion_);
    if (!error.IsSuccess()) { return error; }

    if(appDesc.ApplicationParameters != NULL)
    {
        size_t maxParamLength = ServiceModelConfig::GetConfig().MaxApplicationParameterLength;
        for(ULONG i = 0; i < appDesc.ApplicationParameters->Count; i++)
        {
            wstring name;
            error = StringUtility::LpcwstrToWstring2(appDesc.ApplicationParameters->Items[i].Name, false, name);
            if (!error.IsSuccess()) { return error; }

            wstring value;
            error = StringUtility::LpcwstrToWstring2(appDesc.ApplicationParameters->Items[i].Value, true, 0, maxParamLength, value);
            if (!error.IsSuccess()) { return error; }

            parameterList_[name] = value;
        }
    }

    if (appDesc.Reserved != NULL)
    {
        FABRIC_APPLICATION_DESCRIPTION_EX1 * appDescEx1 = static_cast<FABRIC_APPLICATION_DESCRIPTION_EX1*>(appDesc.Reserved);
        if (appDescEx1->ApplicationCapacity != NULL)
        {
            applicationCapacity_.FromPublicApi(appDescEx1->ApplicationCapacity);
        }
    }

    return ErrorCode::Success();
}
