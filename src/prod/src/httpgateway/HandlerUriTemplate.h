// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
#define MAKE_HANDLER_CALLBACK( _X_ ) [this](AsyncOperationSPtr const &operation) { this->_X_(operation); }

#define MAKE_SUFFIX_PATH( _COLLECTION_, _ACTION_ ) *(_COLLECTION_) + *Constants::MetadataSegment + *(_ACTION_)

    //
    // Starting Api version v3, we support serializing enum's as string and
    // TimeSpan in ISO 8601 format.
    //
    class HandlerUriTemplate
    {
    public:
        HandlerUriTemplate()
            : handlerFunction_(nullptr)
            , minApiVersion_(Constants::V1ApiVersion)
            , maxApiVersion_(HandlerUriTemplate::defaultMaxApiVersion_)
            , allowAnonymous_(false)
            , requireApiVersion_(true)
        {
        }

        HandlerUriTemplate(
                std::wstring const &suffixPath,
                Common::GlobalWString const &verb,
                Common::AsyncCallback const &handleCallback)
            : suffixPath_(suffixPath)
            , verb_(*verb)
            , handlerFunction_(handleCallback)
            , minApiVersion_(Constants::V1ApiVersion)
            , maxApiVersion_(HandlerUriTemplate::defaultMaxApiVersion_)
            , allowAnonymous_(false)
            , requireApiVersion_(true)
        {
        }

        HandlerUriTemplate(
                std::wstring const &suffixPath,
                Common::GlobalWString const &verb,
                Common::AsyncCallback const &handleCallback,
                double minApiVersion,
                double maxApiVersion,
                bool allowAnonymous = false)
            : suffixPath_(suffixPath)
            , verb_(*verb)
            , handlerFunction_(handleCallback)
            , minApiVersion_(minApiVersion)
            , maxApiVersion_(maxApiVersion)
            , allowAnonymous_(allowAnonymous)
            , requireApiVersion_(true)
        {
        }

        HandlerUriTemplate(
                std::wstring const &suffixPath,
                Common::GlobalWString const &verb,
                Common::AsyncCallback const &handleCallback,
                double minApiVersion,
                bool allowAnonymous = false)
            : suffixPath_(suffixPath)
            , verb_(*verb)
            , handlerFunction_(handleCallback)
            , minApiVersion_(minApiVersion)
            , maxApiVersion_(HandlerUriTemplate::defaultMaxApiVersion_)
            , allowAnonymous_(allowAnonymous)
            , requireApiVersion_(true)
        {
        }

        HandlerUriTemplate(
            std::wstring const &suffixPath,
            Common::GlobalWString const &verb,
            Common::AsyncCallback const &handleCallback,
            bool requireApiVersion)
            : suffixPath_(suffixPath)
            , verb_(*verb)
            , handlerFunction_(handleCallback)
            , minApiVersion_(Constants::V1ApiVersion)
            , maxApiVersion_(HandlerUriTemplate::defaultMaxApiVersion_)
            , allowAnonymous_(false)
            , requireApiVersion_(requireApiVersion)
        {
        }

        HandlerUriTemplate(
                std::wstring const &suffixPath,
                Common::GlobalWString const &verb,
                Common::AsyncCallback const &handleCallback,
                bool requireApiVersion,
                bool allowAnonymous)
            : suffixPath_(suffixPath)
            , verb_(*verb)
            , handlerFunction_(handleCallback)
            , minApiVersion_(Constants::V1ApiVersion)
            , maxApiVersion_(HandlerUriTemplate::defaultMaxApiVersion_)
            , allowAnonymous_(allowAnonymous)
            , requireApiVersion_(requireApiVersion)
        {
        }

        ~HandlerUriTemplate() {};

        __declspec(property(get=get_suffixPath, put=set_suffixPath)) std::wstring &SuffixPath;
        __declspec(property(get=get_minApiVersion, put=set_minApiVersion)) double &MinApiVersion;
        __declspec(property(get=get_maxApiVersion, put=set_maxApiVersion)) double &MaxApiVersion;
        __declspec(property(get=get_verb, put=set_verb)) std::wstring &Verb;
        __declspec(property(get=get_callback, put=set_callback)) Common::AsyncCallback &HandlerCallback;
        __declspec(property(get=get_allowAnonymous, put=set_allowAnonymous)) bool AllowAnonymous;
        __declspec(property(get = get_requireApiVersion, put = set_requireApiVersion)) bool RequireApiVersion;

        std::wstring const& get_suffixPath() const{ return suffixPath_; }
        double const& get_minApiVersion() const{ return minApiVersion_; }
        double const& get_maxApiVersion() const{ return maxApiVersion_; }
        std::wstring const& get_verb() const{ return verb_; }
        Common::AsyncCallback const& get_callback() const{ return handlerFunction_; }
        bool const get_allowAnonymous() const{ return allowAnonymous_; }
        bool const get_requireApiVersion() const { return requireApiVersion_; }

        void set_suffixPath(std::wstring const &arg) { suffixPath_ = arg; }
        void set_minApiVersion(double const &arg) { minApiVersion_ = arg; }
        void set_maxApiVersion(double const &arg) { maxApiVersion_ = arg; }
        void set_verb(std::wstring const&arg) { verb_ = arg; }
        void set_callback(Common::AsyncCallback const&arg) { handlerFunction_ = arg; }
        void set_allowAnonymous(bool arg) { allowAnonymous_ = arg; }
        void set_requireApiVersion(bool arg) { requireApiVersion_ = arg; }

        // Default MaxApiVersion = currentRuntimeVersion (majorVersion.minorVersion)
        static double GetDefaultMaxApiVersion();

    private:
        std::wstring verb_;
        std::wstring suffixPath_; // the Suffix path
        static double defaultMaxApiVersion_;
        double minApiVersion_;
        double maxApiVersion_; // this is inclusive.
        Common::AsyncCallback handlerFunction_; // the function that can process requests to this uri.
        bool allowAnonymous_;
        bool requireApiVersion_;
    };
}
