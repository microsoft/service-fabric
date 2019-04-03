// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType_Parser("Parser");

ErrorCode Parser::ParseServicePackage(
    wstring const & fileName, 
    __out ServicePackageDescription & servicePackage)
{
    return ParseElement<ServicePackageDescription>(
        fileName,
        L"ServicePackage",
        servicePackage);
}

ErrorCode Parser::ParseServiceManifest(
    wstring const & fileName,
    __out ServiceManifestDescription & serviceManifest)
{
    return ParseElement<ServiceManifestDescription>(
        fileName,
        L"ServiceManifest",
        serviceManifest);
}

ErrorCode Parser::ParseConfigSettings(
    wstring const & fileName, 
    __out ConfigSettings & configSettings)
{
    return ParseElement<ConfigSettings>(
        fileName,
        L"ConfigSettings",
        configSettings);
}

ErrorCode Parser::ParseApplicationManifest(
    std::wstring const & fileName, 
    __out ApplicationManifestDescription & applicationManifest)
{
    return ParseElement<ApplicationManifestDescription>(
            fileName,
            L"ApplicationManifest",
            applicationManifest);
}

ErrorCode Parser::ParseApplicationPackage(
    std::wstring const & fileName,
    __out ApplicationPackageDescription & applicationPackage)
{
    auto error = ParseElement<ApplicationPackageDescription>(
        fileName,
        L"ApplicationPackage",
        applicationPackage);

    return error;
}

ErrorCode Parser::ParseApplicationInstance(
    std::wstring const & fileName, 
    __out ApplicationInstanceDescription & applicationInstance)
{
    return ParseElement<ApplicationInstanceDescription>(
            fileName,
            L"ApplicationInstance",
            applicationInstance);
}

ErrorCode Parser::ParseInfrastructureDescription(
    std::wstring const & fileName, 
    __out InfrastructureDescription & infrastructureDescription)
{
    return ParseElement<InfrastructureDescription>(
            fileName,
            L"InfrastructureInformation",
            infrastructureDescription);
}

ErrorCode Parser::ParseTargetInformationFileDescription(
    std::wstring const & fileName,
    __out TargetInformationFileDescription & targetInformationFileDescription)
{
    return ParseElement<TargetInformationFileDescription>(
        fileName,
        L"TargetInformation",
        targetInformationFileDescription);
}

void Parser::ThrowInvalidContent(
    XmlReaderUPtr const & xmlReader,
    wstring const & expectedContent, 
    wstring const & actualContent)
{
    UINT lineNo = xmlReader->GetLineNumber();
    UINT linePos = xmlReader->GetLinePosition();

    WriteError(
        TraceType_Parser,
        "Invalid content, expected {0}, found {1}. Input={2}, Line={3}, Position={4}",
        expectedContent,
        actualContent,
        xmlReader->FileName,
        lineNo,
        linePos);

    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
}

void Parser::ThrowInvalidContent(
    XmlReaderUPtr const & xmlReader,
    wstring const & expectedContent, 
    wstring const & actualContent,
    wstring const & reason)
{
    UINT lineNo = xmlReader->GetLineNumber();
    UINT linePos = xmlReader->GetLinePosition();

    WriteError(
        TraceType_Parser,
        "Invalid content, expected {0}, found {1}. Reason={5}, Input={2}, Line={3}, Position={4}",
        expectedContent,
        actualContent,
        xmlReader->FileName,
        lineNo,
        linePos,
        reason);

    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
}

template <typename ElementType>
ErrorCode Parser::ParseElement(
    wstring const & fileName,
    wstring const & elementTypeName,
    __out ElementType & element)
{
    auto error = ErrorCode(ErrorCodeValue::Success);

    try
    {
        XmlReaderUPtr xmlReader;
        error = XmlReader::Create(fileName, xmlReader);
        if (error.IsSuccess())
        {
            element.ReadFromXml(xmlReader);
        }
    }
    catch(XmlException e)
    {
        error = e.Error;
    }

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_Parser,
            "Parse{0}: ErrorCode={1}, FileName={2}",
            elementTypeName,
            error,
            fileName);
    }
    return error;
}

void Parser::Utility::ReadPercentageAttribute(
    XmlReaderUPtr const & xmlReader, 
    wstring const & attrName,
    byte & value)
{
    wstring attrValue = xmlReader->ReadAttributeValue(attrName);
    int intValue = 0;
    
    if (!StringUtility::TryFromWString<int>(attrValue, intValue))
    {
        Parser::ThrowInvalidContent(xmlReader, L"percentage[0-100]", attrValue);
    }
    
    if (intValue < 0 || intValue > 100)
    {
        Parser::ThrowInvalidContent(xmlReader, L"percentage[0-100]", attrValue);
    }
    
    value = (byte)intValue;
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName, 
    __in std::wstring const & paramName, 
    __out wstring & value, 
    __out bool & hasValue)
{
    LPCWSTR configValue = nullptr;
    BOOLEAN isEncrypted = FALSE;

    HRESULT hr = configPackageCPtr->GetValue(sectionName.c_str(), paramName.c_str(), &isEncrypted, &configValue);
    
    if (hr == FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND)
    {
        hasValue = false;
        return ErrorCode::Success();
    }

    ASSERT_IF(isEncrypted == TRUE, "ReplicatorSettings value cannot be encrypted");

    if (FAILED(hr))
    {
        if (hr == FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND)
        {
            wstring errorMessage;
            StringWriter messageWriter(errorMessage);

            messageWriter.Write("Failed to find section {0} in configuration package name = {1}", sectionName, configPackageCPtr->get_Description()->Name);
            ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);
        }
        return ErrorCode::FromHResult(hr);
    }

    value = wstring(configValue);
    hasValue = true;

    return ErrorCode::Success();
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName, 
    __in std::wstring const & paramName, 
    __out int64 & value, 
    __out bool & hasValue)
{
    wstring configValue;
    hasValue = false;
    value = 0;

    ErrorCode error = ReadSettingsValue(configPackageCPtr, sectionName, paramName, configValue, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (!hasValue) { return error; }

    if (!Config::TryParse<int64>(value, configValue))
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write("Failed to parse config entry {0} with value {1} in section {2}. Config Package Name = {3}", paramName, configValue, sectionName, configPackageCPtr->get_Description()->Name);
        ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);

        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    hasValue = true;
    return ErrorCode::Success();
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName, 
    __in std::wstring const & paramName, 
    __out int & value, 
    __out bool & hasValue)
{
    wstring configValue;
    hasValue = false;
    value = 0;

    ErrorCode error = ReadSettingsValue(configPackageCPtr, sectionName, paramName, configValue, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (!hasValue) { return error; }

    if (!Config::TryParse<int>(value, configValue))
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write("Failed to parse config entry {0} with value {1} in section {2}. Config Package Name = {3}", paramName, configValue, sectionName, configPackageCPtr->get_Description()->Name);
        ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);

        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    hasValue = true;
    return ErrorCode::Success();
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName, 
    __in std::wstring const & paramName, 
    __out TimeSpan & value, 
    __out bool & hasValue)
{
    wstring configValue;
    hasValue = false;
    value = TimeSpan::Zero;

    ErrorCode error = ReadSettingsValue(configPackageCPtr, sectionName, paramName, configValue, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (!hasValue) { return error; }

    StringUtility::TrimWhitespaces(configValue);
    StringUtility::ToLower(configValue);
    
    if (!Config::TryParse<TimeSpan>(value, configValue))
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write("Failed to parse config entry {0} with value {1} in section {2}. Config Package Name = {3}", paramName, configValue, sectionName, configPackageCPtr->get_Description()->Name);
        ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);

        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    hasValue = true;
    return ErrorCode::Success();
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName, 
    __in std::wstring const & paramName, 
    __out bool & value, 
    __out bool & hasValue)
{
    wstring configValue;
    hasValue = false;
    value = FALSE;

    ErrorCode error = ReadSettingsValue(configPackageCPtr, sectionName, paramName, configValue, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (!hasValue) { return error; }

    StringUtility::TrimWhitespaces(configValue);
    StringUtility::ToLower(configValue);

    if (configValue == L"true")
    {
        value = TRUE;
        hasValue = true;
    }
    else if (configValue == L"false")
    {
        value = FALSE;
        hasValue = true;
    }
    else
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write("Failed to parse config entry {0} with value {1} in section {2}. Config Package Name = {3}", paramName, configValue, sectionName, configPackageCPtr->get_Description()->Name);
        ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);

        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    return ErrorCode::Success();
}

ErrorCode Parser::ReadSettingsValue(
    __in ComPointer<IFabricConfigurationPackage2> configPackageCPtr,
    __in std::wstring const & sectionName,
    __in std::wstring const & paramPrefix,
    __out std::map<wstring, wstring> & values,
    __out bool & hasValue)
{
    FABRIC_CONFIGURATION_PARAMETER_LIST *configValues;

    HRESULT hr = configPackageCPtr->GetValues(sectionName.c_str(), paramPrefix.c_str(), &configValues);

    if (hr == FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND)
    {
        hasValue = false;
        return ErrorCode::Success();
    }

    if (FAILED(hr))
    {
        if (hr == FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND)
        {
            wstring errorMessage;
            StringWriter messageWriter(errorMessage);

            messageWriter.Write("Failed to find section {0} in configuration package name = {1}", sectionName, configPackageCPtr->get_Description()->Name);
            ServiceModelEventSource::Trace->ConfigurationParseError(errorMessage);
        }
        return ErrorCode::FromHResult(hr);
    }

    for (size_t ix = 0; ix < configValues->Count; ++ix)
    {
        ASSERT_IF(configValues->Items[ix].IsEncrypted == TRUE, "ReplicatorSettings value cannot be encrypted");

        wstring parameterNameWithoutPrefix = wstring(configValues->Items[ix].Name).substr(paramPrefix.size());
        values.insert(std::pair<wstring, wstring>(parameterNameWithoutPrefix, wstring(configValues->Items[ix].Value)));
    }

    hasValue = true;
    return ErrorCode::Success();
}

ErrorCode Parser::IsServiceManifestFile(
    wstring const & fileName,
    __out bool & result)
{
    result = false;
    ErrorCode error(ErrorCodeValue::Success);
    try
    {
        XmlReaderUPtr xmlReader;
        error = XmlReader::Create(fileName, xmlReader);
        if (error.IsSuccess())
        {
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ServiceManifest,
                *SchemaNames::Namespace))
            {
                result = true;
            }
        }
    }
    catch (XmlException e)
    {
        error = e.Error;
    }
    return error;
}

template <class T>
Common::ErrorCode Parser::ParseContinuationToken(
    std::wstring const & continuationToken,
    __out T & continuationTokenObj)
{
    // url decode the continuation token string
    wstring continuationTokenDecoded;
    auto error = Common::NamingUri::UnescapeString(continuationToken, continuationTokenDecoded);

    if (!error.IsSuccess)
    {
        return error;
    }

    // If it's empty, then return an empty error
    if (continuationTokenDecoded.empty())
    {
        return Common::ErrorCode(Common::ErrorCodeValue::ArgumentNull, wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }

    // Try to parse as a JSON
    // The deserializer does not require that all fields be present. Thus, we need to check validity manually
    // to make sure that we don't have a continuation token with no parts or only one part.
    error = Common::JsonHelper::Deserialize(continuationTokenObj, continuationTokenDecoded);

    if (!continuationTokenObj.IsValid() || error.IsError(Common::ErrorCodeValue::SerializationError))
    {
        return Common::ErrorCode(
            Common::ErrorCodeValue::InvalidArgument,
            Common::wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationTokenDecoded));
    }

    return error;
}

// This is only needed until deprecate the "+" form of delimiter. See Bug # 9730099
// Replace with declaration of ApplicationTypeQueryContinuationToken
// template�<>
// Common::ErrorCode�Parser::ParseContinuationToken<ApplicationTypeQueryContinuationToken>(
// std::wstring const & continuationToken,
// __out ApplicationTypeQueryContinuationToken & continuationTokenObj)

template <>
ErrorCode Parser::ParseContinuationToken<ApplicationTypeQueryContinuationToken>(
    std::wstring const & continuationToken,
    __out ApplicationTypeQueryContinuationToken & continuationTokenObj)
{
    // url decode the continuation token string
    wstring continuationTokenDecoded;
    auto error = NamingUri::UnescapeString(continuationToken, continuationTokenDecoded);

    if (!error.IsSuccess())
    {
        return error;
    }

    // If it's empty, then return an empty error
    if (continuationTokenDecoded.empty())
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull, wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationToken));
    }

    // Try to parse as a JSON
    // The deserializer does not require that all fields be present. Thus, we need to check validity manually
    // to make sure that we don't have a continuation token with no parts or only one part.
    error = JsonHelper::Deserialize(continuationTokenObj, continuationTokenDecoded);

    if (error.IsError(ErrorCodeValue::SerializationError))
    {
        // If the code has reached here, then we know that we don't have a JSON object
        // Take care of the "+" case
        vector<std::wstring> continuationTokenList;
        StringUtility::Split(continuationToken, continuationTokenList, Constants::ContinuationTokenParserDelimiter);

        // Continuation tokens that are passed in here should not be empty
        // And valid continuation tokens should have 2 segments
        if (continuationTokenList.size() != 2 || continuationTokenList[0].empty() || continuationTokenList[1].empty())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationTokenDecoded));
        }

        continuationTokenObj = move(ApplicationTypeQueryContinuationToken(move(continuationTokenList[0]), move(continuationTokenList[1])));

        return ErrorCode(ErrorCodeValue::Success);
    }

    if (!continuationTokenObj.IsValid())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Continuation_Token), continuationTokenDecoded));
    }

    return error;
}
