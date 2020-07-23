// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpApplicationGateway;
using namespace HttpServer;
using namespace Naming;
using namespace HttpCommon;

bool ParsedGatewayUri::TryParse(IRequestMessageContextUPtr &messageContext, __out ParsedGatewayUriSPtr &parsedGatewayUriSPtr, std::wstring serviceName)
{
    wstring requestUri = messageContext->GetUrl();

    //
    // Parse the url string. Parser in Common::Uri validates the Uri's well formedness
    //
    Uri uri;
    if (!Uri::TryParse(requestUri, uri)) 
    { 
        return false; 
    }

    auto parsedGatewayUri = shared_ptr<ParsedGatewayUri>(new(messageContext->GetThisAllocator()) ParsedGatewayUri(uri));
    if (!parsedGatewayUri->TryParseQueryParams(uri.Query)) 
    { 
        return false; 
    }

    if (serviceName.empty())
    {
        // Get the UrlSuffix path.
        parsedGatewayUri->ParseSuffix(messageContext->GetSuffix());
    }
    else
    {
        parsedGatewayUri->serviceName_ = serviceName;

        // Get the UrlSuffix path.
        parsedGatewayUri->ParseHttpGatewayUriSuffix(messageContext->GetSuffix());
    }

    parsedGatewayUriSPtr = move(parsedGatewayUri);
    return true;
}

bool ParsedGatewayUri::GetUriForForwarding(
    wstring const &serviceName,
    wstring const &serviceListenAddress,
    __out wstring &forwardingUri)
{
    wstring escapedServiceName;
    auto error = NamingUri::FabricNameToId(serviceName, escapedServiceName);
    if (!error.IsSuccess())
    {
        Assert::CodingError("FabricNameToId failed with {0}", error);
    }

    wstring originalSuffix = urlSuffix_;
    // Remove the URI portion after the service name in the incoming uri suffix.
    if (StringUtility::StartsWith(originalSuffix, escapedServiceName))
    {
        originalSuffix.erase(0, escapedServiceName.size());
    }

    // Construct the final forwarding uri
    wstring uriBuffer;
    wstring trimmedServiceListenAddress = serviceListenAddress;
    StringUtility::TrimTrailing<wstring>(trimmedServiceListenAddress, L"/");

    StringWriter writer(uriBuffer);
    if (StringUtility::StartsWith(originalSuffix, L"/"))
    {
        writer.Write("{0}{1}", trimmedServiceListenAddress, originalSuffix);
    }
    else
    {
        writer.Write("{0}/{1}", trimmedServiceListenAddress, originalSuffix);
    }

    if (!parsedQueryString_.empty())
    {
        uriBuffer = uriBuffer + L"?" + parsedQueryString_;
    }

    if (!uri_.Fragment.empty())
    {
        uriBuffer = uriBuffer + L"#" + uri_.Fragment;
    }

    if (!HttpUtil::IsHttpUri(uriBuffer) && !HttpUtil::IsHttpsUri(uriBuffer))
    {
        return false;
    }

    forwardingUri = uriBuffer;

    return true;
}

ErrorCode ParsedGatewayUri::TryGetServiceName(__out NamingUri &serviceNameUri)
{
    ErrorCode error;

    if (!serviceNameQueryParam_.empty())
    {
        wstring unescapedName;
        error = NamingUri::UnescapeString(serviceNameQueryParam_, unescapedName);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (!NamingUri::TryParse(unescapedName, serviceNameUri))
        {
            return ErrorCodeValue::InvalidNameUri;
        }
    }
    else if (!serviceName_.empty())
    {
        error = Utility::SuffixToFabricUri(serviceName_, serviceNameUri);
    }
    else
    {
        //
        // If the user didnt provide an explicit servicename, we assume that the servicename is
        // a part of the url suffix and resolve the service endpoint via prefix matching.
        //
        error = Utility::SuffixToFabricUri(RequestUrlSuffix, serviceNameUri);
    }

    return error;
}

bool ParsedGatewayUri::TryParseQueryParams(wstring const &queryString)
{
    StringCollection queryElements;
    StringUtility::Split<wstring>(queryString, queryElements, L"&");
    wstring partitionKey;
    wstring partitionKind;

    for (auto & itr : queryElements)
    {
        wstring name;
        wstring value;
        StringUtility::SplitOnce<wstring, wchar_t>(itr, name, value, L'=');

        //
        // Look for PartitionKey/PartitionScheme/Timeout/ServiceName
        //
        if (name == *Constants::PartitionKey)
        {
            partitionKey = move(value);
        }
        else if (name == *Constants::PartitionKind)
        {
            partitionKind = move(value);
        }
        else if (name == *Constants::ServiceName)
        {
            serviceNameQueryParam_ = move(value);
        }
        else if (name == *Constants::TargetReplicaSelectorString)
        {
            auto error = TargetReplicaSelector::EnumFromString(value, targetReplicaSelector_);
            if (!error.IsSuccess())
            {
                // TODO: Trace
                return false;
            }
        }
        else if (name == *Constants::ListenerName)
        {
            listenerName_ = move(value);
        }
        else
        {
            if (name == *Constants::TimeoutString)
            {
                int64 timeoutInSeconds;
                if (!Common::StringUtility::TryFromWString<int64>(value, timeoutInSeconds, 10))
                {
                    return false;
                }

                requestTimeout_ = TimeSpan::FromSeconds((double)timeoutInSeconds);

                // Timeout need not be removed from the query params forwarded to the service
                // Hence appending it to parsedQueryString_ below
                // TODO: Do remaining time calculation and update this timeout when
                // doing the actual forwarding.
            }

            if (!parsedQueryString_.empty())
            {
                parsedQueryString_.append(L"&");
            }

            parsedQueryString_.append(itr);
        }
    }

    if (!TryParsePartitionKindQueryParam(partitionKind, partitionKey))
    {
        return false;
    }

    return true;
}

bool ParsedGatewayUri::TryParsePartitionKindQueryParam(wstring const &partitionKind, wstring const &partitionKey)
{
    FABRIC_PARTITION_KEY_TYPE keyType;
    void *key = nullptr;
    int64 int64Key;
    if (!partitionKind.empty())
    {
        if (partitionKind == *Constants::PartitionKindNamed)
        {
            keyType = FABRIC_PARTITION_KEY_TYPE_STRING;
            key = (void *)partitionKey.c_str();
        }
        else if (partitionKind == *Constants::PartitionKindInt64)
        {
            keyType = FABRIC_PARTITION_KEY_TYPE_INT64;
            if (!Common::StringUtility::TryFromWString<int64>(partitionKey, int64Key, 10))
            {
                return false;
            }

            key = (void *)(&int64Key);
        }
        else if (partitionKind == *Constants::PartitionKindSingleton)
        {
            keyType = FABRIC_PARTITION_KEY_TYPE_NONE;
        }
        else
        {
            return false;
        }
    }
    else
    {
        keyType = FABRIC_PARTITION_KEY_TYPE_NONE;
    }

    auto error = partitionKey_.FromPublicApi(keyType, key);
    if (!error.IsSuccess())
    {
        return false;
    }

    return true;
}

void ParsedGatewayUri::ParseSuffix(wstring const &suffixElement)
{
    //
    // Remove the query string and fragment. Fragment is always the last portion of the URI, so splitting by
    // query string is sufficient.
    //
    StringCollection uriElements;
    if (suffixElement.length() > 0)
    {
        StringUtility::Split<wstring>(suffixElement, uriElements, *Constants::QueryStringDelimiter, false);
    }

    if (uriElements.size() > 0)
    {
        urlSuffix_ = uriElements[0];
    }
}

void ParsedGatewayUri::ParseHttpGatewayUriSuffix(wstring const &suffixElement)
{
    //
    // Remove the query string and fragment. Fragment is always the last portion of the URI, so splitting by
    // query string is sufficient.
    //
    StringCollection uriElements;
    if (suffixElement.length() > 0)
    {
        StringUtility::Split<wstring>(suffixElement, uriElements, *Constants::QueryStringDelimiter, false);
    }

    if (uriElements.size() > 0)
    {
        // TODO: Remove the hardcoding from here and make this piece generic for services to register a entity and forward all URLs under the same to them.
        // For Backup restore service, the entity is BackupRestore.

        wstring backupRestorePrefix = L"BackupRestore/";
        wstring eventsStorePrefix = L"EventsStore/";
        // If it begins with BackupRestore (Backup Restore service URI), http://ip:port/BackupRestore/path1/path2/$/path3
        // urlSuffix_ = path1/path2/$/path3
        if (StringUtility::StartsWithCaseInsensitive(uriElements[0], backupRestorePrefix))
        {
            wstring backuprestoreStr;
            wstring urlSuffix;
            StringUtility::SplitOnce<wstring, wchar_t>(uriElements[0], backuprestoreStr, urlSuffix, L'/');
            urlSuffix_ = urlSuffix;
        }
        // Use the whole uri to EventStoreService
        else if (StringUtility::StartsWithCaseInsensitive(uriElements[0], eventsStorePrefix))
        {
            urlSuffix_ = uriElements[0];
        }
        else
        {
            // If this is a custom HttpGateway URI (not the generic system reverse proxy URI), compute the suffix
            // Eg: http://ip:port/Applications/App1/$/GetProtection 
            // urlSuffix_ = GetProtection
            StringCollection uriMetadataSegmentElements;
            StringUtility::SplitOnString<wstring>(uriElements[0], uriMetadataSegmentElements, L"/$/", false);
            urlSuffix_ = uriMetadataSegmentElements.back();
        }
    }
}
