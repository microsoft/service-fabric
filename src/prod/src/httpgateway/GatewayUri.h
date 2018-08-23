// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // This class is used by all the REST Uri handlers to validate and parse
    // the http request url.
    //
    class GatewayUri
    {
        DEFAULT_COPY_CONSTRUCTOR(GatewayUri);
             
    public:

        static Common::GlobalWString metadataSegment;
        static Common::GlobalWString itemDelimiterStart;
        static Common::GlobalWString itemDelimiterEnd;

        //
        // A URI can contain collections, keys or meta data identifiers like functions, actions or typenames 
        //
        enum UriSegments
        {
            UriInvalid = 0,
            UriCollection = 1,
            UriCollectionKey = 2,
            UriFunction = 3,
        };

        GatewayUri() {};
        ~GatewayUri() {};

        __declspec(property(get=get_query)) std::wstring &Query;
        __declspec(property(get=get_verb)) std::wstring &Verb;
        __declspec(property(get=get_callback)) Common::AsyncCallback &Handler;
        __declspec(property(get=get_allowAnonymous)) bool AllowAnonymous;
        __declspec(property(get = get_ApiVersion)) double const &ApiVersion;

        std::wstring const& get_query() const { return query_; }
        std::wstring const& get_verb() const { return verb_; }
        Common::AsyncCallback const& get_callback() const{ return handler_; }
        bool get_allowAnonymous() const { return allowAnonymous_; }
        double const& get_ApiVersion() const { return apiVersion_; }

        static bool TryParse(
            __in std::vector<HandlerUriTemplate> const& validHandlerUri,
            __in std::wstring const& verb,
            __in std::wstring const& url,
            __in std::wstring const& suffix,
            __out GatewayUri &output,
            __in bool allowHierarchicalEntityKeys = true);
	
        GatewayUri & operator = (GatewayUri && other);       
        bool GetItem(__in std::wstring const&, __out_opt std::wstring &) const;

        Common::ErrorCode GetBool(__in std::wstring const&, __out_opt bool &) const;

    private:

        bool Parse(std::vector<HandlerUriTemplate> const& validHandlerUri, std::wstring const &suffix, std::wstring const& query, std::wstring const&verb, bool allowHierarchicalEntityKeys);
        
        bool ParseHierarchical(
            HandlerUriTemplate const& validHandlerUri,
            std::wstring const& verb,
            Common::StringCollection const& requestSuffix,
            std::wstring const& requestQuery);

        bool ParseNonHierarchical(
            HandlerUriTemplate const& validHandlerUri,
            std::wstring const& verb,
            Common::StringCollection const& requestSuffix,
            std::wstring const& requestQuery);

        bool UpdateKey(std::wstring &keyName, std::wstring &keyValue, std::wstring const &handlerToken, std::wstring const &suffixToken);
        void ParseQueryString(std::wstring const& queryString);

        std::wstring GetKey(std::wstring const &keyWithDelimiters);
        bool IsKey(std::wstring const &segment);

        //
        // No need for locking this map. Inserts into this map happen only
        // during the construction of a gatewayUri object and this is read only
        // after that point.
        //
        typedef std::map<std::wstring, std::wstring> UriItems;
        UriItems items_;
        std::wstring query_;
        std::wstring verb_;
        Common::AsyncCallback handler_;
        bool allowAnonymous_;
        double apiVersion_;
    };
}
