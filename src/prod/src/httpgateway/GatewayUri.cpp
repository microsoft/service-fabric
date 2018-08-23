// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

Common::GlobalWString GatewayUri::metadataSegment = Common::make_global<std::wstring>(L"$");
Common::GlobalWString GatewayUri::itemDelimiterStart = Common::make_global<std::wstring>(L"{");
Common::GlobalWString GatewayUri::itemDelimiterEnd = Common::make_global<std::wstring>(L"}");

GatewayUri  & GatewayUri::operator = (GatewayUri && other)
{
    if (this != &other)
    {
        items_ = move(other.items_);
        query_ = move(other.query_);
        verb_ = move(other.verb_);
        handler_ = move(other.handler_);
        allowAnonymous_ = move(other.allowAnonymous_);
        apiVersion_ = move(other.apiVersion_);
    }

    return *this;
}

bool GatewayUri::TryParse(
    __in vector<HandlerUriTemplate> const& validHandlerUris,
    __in wstring const& verb,
    __in wstring const& url,
    __in wstring const& suffix,
    __out GatewayUri &output,
    __in bool allowHierarchicalEntityKeys)
{
    //
    // Confirm if the suffix is indeed a part of the Path
    //
    if (!StringUtility::Contains(url, suffix)) { return false; }

    //
    // Parse the url string. Parser in Common::Uri validates the Uri's well formedness
    //
    Uri uri;
    if (!Uri::TryParse(url, uri)) { return false; }

    GatewayUri gatewayuri;
    if (!gatewayuri.Parse(validHandlerUris, suffix, uri.Query, verb, allowHierarchicalEntityKeys)) { return false; }

    output = std::move(gatewayuri);
    output.query_ = uri.Query;
    return true;
}

bool GatewayUri::Parse(vector<HandlerUriTemplate> const& validHandlerUris, wstring const& suffix, wstring const& requestQuery, wstring const& verb, bool allowHierarchicalEntityKeys)
{
    bool result = false;

    //
    // The URI format has already been validated before we get here, split the query and fragment portions
    // from the actual suffix.
    //
    StringCollection suffixUriVector;
    StringUtility::Split<wstring>(suffix, suffixUriVector, *Constants::QueryStringDelimiter, false);

    StringCollection requestSuffixVector;
    if (suffixUriVector.size() >= 1)
    {
        // check for fragment portion
        wstring temp = suffixUriVector[0];
        suffixUriVector.clear();
        StringUtility::Split<wstring>(temp, suffixUriVector, L"#", false);

        if (suffixUriVector.size() >= 1)
        {
            StringUtility::TrimTrailing<wstring>(suffixUriVector[0], Constants::SegmentDelimiter);
            StringUtility::Split<wstring>(suffixUriVector[0], requestSuffixVector, Constants::SegmentDelimiter);
        }
    }

    for (size_t i = 0; i < validHandlerUris.size(); i++)
    {
        if (allowHierarchicalEntityKeys)
        {
            result = ParseHierarchical(validHandlerUris[i], verb, requestSuffixVector, requestQuery);
        }
        else
        {
            result = ParseNonHierarchical(validHandlerUris[i], verb, requestSuffixVector, requestQuery);
        }

        if (result)
        {
            break;
        }
    }

    return result;
}

bool GatewayUri::ParseHierarchical(
    HandlerUriTemplate const& validHandlerUri,
    wstring const& verb,
    StringCollection const& requestSuffix,
    wstring const& requestQuery)
{
    bool result = true;
    StringCollection tokenizedHandlerPath;
    wstring currentKeyName;
    wstring currentKeyValue;
    StringUtility::Split<wstring>(validHandlerUri.SuffixPath, tokenizedHandlerPath, Constants::SegmentDelimiter);
    StringCollection::iterator handlerItr = tokenizedHandlerPath.begin();
    StringCollection::const_iterator suffixPathItr = requestSuffix.begin();

    if ((verb != validHandlerUri.Verb) ||
        (tokenizedHandlerPath.size() > requestSuffix.size()))
    {
        return false;
    }

    //
    // check the top level collection
    // 
    wstring handlerCollectionBase;
    wstring suffixPathBase;
    if (!tokenizedHandlerPath.empty())
    {
        handlerCollectionBase = *handlerItr;
    }
    if (!requestSuffix.empty())
    {
        suffixPathBase = *suffixPathItr;
    }
    if (handlerCollectionBase != suffixPathBase)
    {
        return false;
    }

    // NOTE: For Backup Restore and Events Store prefixes, do not parse since the suffix format may not be in conformance with the gateway URI schema
    // suffix will be passed to the service as is.
    if (suffixPathBase == Constants::BackupRestorePrefix || suffixPathBase == Constants::EventsStorePrefix)
    {
        verb_ = validHandlerUri.Verb;
        handler_ = validHandlerUri.HandlerCallback;
        allowAnonymous_ = validHandlerUri.AllowAnonymous;
        return true;
    }

    //
    // NOTE: Cluster management handler operations are not under any collection. This is the
    // special case to handle that.
    //
    if (!requestSuffix.empty() && *suffixPathItr != metadataSegment)
    {
        // The suffix can start with multiple segments
        while (handlerItr != tokenizedHandlerPath.end()
            && suffixPathItr != requestSuffix.end()
            && (*suffixPathItr == *handlerItr))
        {
            handlerItr++;
            suffixPathItr++;
        }
    }

    //
    // check the suffix.
    //
    UriSegments currentSegment = UriSegments::UriCollectionKey;
    for ( ; (suffixPathItr != requestSuffix.end()) && (result == true); ++suffixPathItr)
    {
        switch (currentSegment)
        {
            case UriSegments::UriCollectionKey:
            {
                //
                // Check this is a switch to metadata namespace.
                //
                if (*suffixPathItr == metadataSegment)
                {
                    if (handlerItr != tokenizedHandlerPath.end() && *handlerItr == metadataSegment)
                    {
                        currentSegment = UriSegments::UriFunction;
                        if (!currentKeyName.empty()) 
                        {
                            items_[currentKeyName] = currentKeyValue;
                            currentKeyName.clear();
                            currentKeyValue.clear();
                        }
                        ++handlerItr;
                    }
                    else
                    {
                        result = false;
                    }
                }
                else
                {
                    wstring handlerToken;
                    if (handlerItr != tokenizedHandlerPath.end())
                    {
                        handlerToken = *handlerItr;
                        if (currentKeyName.empty())
                        {
                            ++handlerItr;
                        }
                    }

                    result = UpdateKey(currentKeyName, currentKeyValue, handlerToken, *suffixPathItr);
                }
                break;
            }
            case UriSegments::UriFunction:
            {
                if (handlerItr == tokenizedHandlerPath.end() || *suffixPathItr != *handlerItr) 
                { 
                    result = false; 
                }
                else
                {
                    ++handlerItr;
                    currentSegment = UriSegments::UriCollectionKey;
                }
                break;
            }
        }
    }

    if (handlerItr != tokenizedHandlerPath.end())
    {
        result = false;
    }

    //
    // Parse and populate the query params
    //
    double apiVersion = 0.0;
    if (result == true)
    {
        if (!requestQuery.empty())
        {
            ParseQueryString(requestQuery);
        }

        if (HttpGatewayConfig::GetConfig().VersionCheck && validHandlerUri.RequireApiVersion)
        {
            if ((items_.count(*Constants::ApiVersionString) == 0))
            {
                result = false;
            }
            else
            {
                //
                // Api's that are in preview have the version format <apiversion>-preview. Eg: 3.0-preview
                //
                auto apiVersionStr = items_[*Constants::ApiVersionString];
                if (StringUtility::EndsWith(apiVersionStr, *Constants::PreviewApiVersion))
                {
                    StringUtility::TrimTrailing(apiVersionStr, *Constants::PreviewApiVersion);
                }

                if (StringUtility::TryFromWString<double>(apiVersionStr, apiVersion) &&
                    ((apiVersion >= validHandlerUri.MinApiVersion) && (apiVersion <= validHandlerUri.MaxApiVersion)))
                {
                    result = true;
                }
                else
                {
                    result = false;
                }
            }
        }
    }

    // cache the successful values.
    if (result == true) 
    {
        verb_ = validHandlerUri.Verb;
        handler_ = validHandlerUri.HandlerCallback;
        allowAnonymous_ = validHandlerUri.AllowAnonymous;
        apiVersion_ = apiVersion;
        if (!currentKeyName.empty())
        {
            items_[currentKeyName] = currentKeyValue;
        }
    }
    else
    {
        items_.clear(); 
    }

    return result;
}

bool GatewayUri::ParseNonHierarchical(
    HandlerUriTemplate const& validHandlerUri,
    wstring const& verb,
    StringCollection const& requestSuffix,
    wstring const& requestQuery)
{
    bool result = true;
    StringCollection tokenizedHandlerPath;
    wstring currentKeyName;
    wstring currentKeyValue;
    StringUtility::Split<wstring>(validHandlerUri.SuffixPath, tokenizedHandlerPath, Constants::SegmentDelimiter);
    StringCollection::iterator handlerItr = tokenizedHandlerPath.begin();
    StringCollection::const_iterator suffixPathItr = requestSuffix.begin();

    //
    // Segment size for Uri's with Non hierarchical resources should exactly match with the uri template
    //
    if ((verb != validHandlerUri.Verb) ||
        (tokenizedHandlerPath.size() != requestSuffix.size()))
    {
        return false;
    }

    while ((handlerItr != tokenizedHandlerPath.end()) && (suffixPathItr != requestSuffix.end()))
    {
        if (IsKey(*handlerItr))
        {
            items_[GetKey(*handlerItr)] = *suffixPathItr;
        }
        else
        {
            if (*handlerItr != *suffixPathItr)
            {
                break;
            }
        }

        ++handlerItr;
        ++suffixPathItr;
    }

    if (handlerItr != tokenizedHandlerPath.end() || suffixPathItr != requestSuffix.end())
    {
        result = false;
    }

    //
    // Parse and populate the query params
    //
    double apiVersion = 0.0;
    if (result == true)
    {
        if (!requestQuery.empty())
        {
            ParseQueryString(requestQuery);
        }

        if (HttpGatewayConfig::GetConfig().VersionCheck && validHandlerUri.RequireApiVersion)
        {
            if ((items_.count(*Constants::ApiVersionString) == 0))
            {
                result = false;
            }
            else
            {
                //
                // Api's that are in preview have the version format <apiversion>-preview. Eg: 3.0-preview
                //
                auto apiVersionStr = items_[*Constants::ApiVersionString];
                if (StringUtility::EndsWith(apiVersionStr, *Constants::PreviewApiVersion))
                {
                    StringUtility::TrimTrailing(apiVersionStr, *Constants::PreviewApiVersion);
                }

                if (StringUtility::TryFromWString<double>(apiVersionStr, apiVersion) &&
                    ((apiVersion >= validHandlerUri.MinApiVersion) && (apiVersion <= validHandlerUri.MaxApiVersion)))
                {
                    result = true;
                }
                else
                {
                    result = false;
                }
            }
        }
    }

    // cache the successful values.
    if (result == true) 
    {
        verb_ = validHandlerUri.Verb;
        handler_ = validHandlerUri.HandlerCallback;
        allowAnonymous_ = validHandlerUri.AllowAnonymous;
        apiVersion_ = apiVersion;
    }
    else
    {
        items_.clear(); 
    }

    return result;
}

bool GatewayUri::UpdateKey(wstring &keyName, wstring &keyValue, wstring const &handlerToken, wstring const &suffixToken)
{
    if (keyName.empty() && handlerToken.empty())
    {
        return false;
    }

    if (keyName.empty())
    {
       wstring temp = handlerToken;
       if (!IsKey(temp))
       {
           return false;
       }

       keyName = GetKey(temp);
       keyValue = suffixToken;
       return true;
    }

    keyValue += *Constants::SegmentDelimiter + suffixToken;

    return true;
}

bool GatewayUri::GetItem(__in wstring const& itemName, __out_opt wstring &value) const
{
    auto itr = items_.find(itemName);
    if (itr == items_.end()) { return false; }
    value = itr->second;
    return true;
}

ErrorCode GatewayUri::GetBool(__in wstring const& itemName, __out_opt bool &value) const
{
    auto itr = items_.find(itemName);
    if (itr == items_.end()) 
    { 
        value = false;
        return ErrorCodeValue::NotFound; 
    }

    wstring const & valueString = itr->second;
    if (valueString == L"true")
    {
        value = true;
        return ErrorCode::Success();
    }
    else if (valueString == L"false")
    {
        value = false;
        return ErrorCode::Success();
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }
}

void GatewayUri::ParseQueryString(wstring const& queryString)
{
    StringCollection queryElements;
    StringUtility::Split<wstring>(queryString, queryElements, L"&");
    for (auto itr = queryElements.begin(); itr != queryElements.end(); ++itr)
    {
        StringCollection nameValuePair;
        StringUtility::Split<wstring>(*itr, nameValuePair, L"=");

        //
        // Ignore any query param names that match an already existing name.
        //
        if (nameValuePair.size() == 2 && items_.count(nameValuePair[0]) == 0)
        {
            items_[nameValuePair[0]] = nameValuePair[1];
        }
    }
}

bool GatewayUri::IsKey(wstring const &segment)
{
   if (!Common::StringUtility::StartsWith(segment, *itemDelimiterStart) ||
       !Common::StringUtility::EndsWith(segment, *itemDelimiterEnd))
   {
       return false;
   }

   return true;
}

wstring GatewayUri::GetKey(wstring const &keyWithDelimiters)
{
    auto temp = keyWithDelimiters;

   StringUtility::Trim<wstring>(temp, itemDelimiterStart);
   StringUtility::Trim<wstring>(temp, itemDelimiterEnd);

   return temp;
}
