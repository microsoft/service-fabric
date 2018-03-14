// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    StringLiteral Uri::TraceCategory("Uri");

    INITIALIZE_SIZE_ESTIMATION(Uri)

    Uri::Uri(Uri && other)
        : type_(std::move(other.type_)),
          scheme_(std::move(other.scheme_)),
          authority_(std::move(other.authority_)),
          hostType_(std::move(other.hostType_)),
          host_(std::move(other.host_)),
          port_(std::move(other.port_)),
          path_(std::move(other.path_)),
          query_(std::move(other.query_)),
          fragment_(std::move(other.fragment_)),
          pathSegments_(std::move(other.pathSegments_))
    {
    }

    Uri::Uri(std::wstring const & scheme)
        : type_(UriType::Empty),
          scheme_(scheme)
    {
        Normalize();
    }

    Uri::Uri(std::wstring const & scheme, std::wstring const & authority)
        : type_(authority.empty() ? UriType::Empty : UriType::AuthorityAbEmpty),
          scheme_(scheme),
          authority_(authority)
    {
        Normalize();
    }

    Uri::Uri(std::wstring const & scheme, std::wstring const & authority, std::wstring const & path)
        : type_(authority.empty() ? (path.empty() ? UriType::Empty : UriType::Absolute) : UriType::AuthorityAbEmpty),
          scheme_(scheme),
          authority_(authority),
          path_(FixPathForConstructor(path)),
          pathSegments_(GetSegments(path))
    {
        Normalize();
    }

    Uri::Uri(
        std::wstring const & scheme,
        std::wstring const & authority,
        std::wstring const & path,
        std::wstring const & query,
        std::wstring const & fragment)
        : type_(authority.empty() ? UriType::Absolute : UriType::AuthorityAbEmpty),
          scheme_(scheme),
          authority_(authority),
          path_(path),
          query_(query),
          fragment_(fragment),
          pathSegments_(GetSegments(path))
    {
        Normalize();
    }

    Uri & Uri::operator = (Uri && other)
    {
        type_ = std::move(other.type_);
        scheme_ = std::move(other.scheme_);
        authority_ = std::move(other.authority_);
        hostType_ = std::move(other.hostType_);
        host_ = std::move(other.host_);
        port_ = std::move(other.port_);
        path_ = std::move(other.path_);
        query_ = std::move(other.query_);
        fragment_ = std::move(other.fragment_);
        pathSegments_ = std::move(other.pathSegments_);

        return *this;
    }

    Uri::~Uri()
    {
    }

    bool Uri::operator == (Uri const & other) const
    {
        return ((type_ == other.type_) &&
                (scheme_ == other.scheme_) &&
                (authority_ == other.authority_) &&
                (path_ == other.path_) &&
                (query_ == other.query_) &&
                (fragment_ == other.fragment_));
    }

    bool Uri::operator != (Uri const & other) const
    {
        return !(*this == other);
    }

    bool Uri::operator < (Uri const & other) const
    {
        return Compare(other) < 0;
    }

    int Uri::Compare(Uri const & other) const
    {
        int comparison = scheme_.compare(other.scheme_);
        if (comparison != 0) { return comparison; }

        comparison = authority_.compare(other.authority_);
        if (comparison != 0) { return comparison; }

        comparison = path_.compare(other.path_);
        if (comparison != 0) { return comparison; }

        comparison = query_.compare(other.query_);
        if (comparison != 0) { return comparison; }

        comparison = fragment_.compare(other.fragment_);
        if (comparison != 0) { return comparison; }

        return 0;
    }

    Uri Uri::GetTrimQueryAndFragment() const
    {
        if (this->Query.empty() && this->Fragment.empty())
        {
            return *this;
        }
        else
        {
            return Uri(this->Scheme, this->Authority, this->Path);
        }
    }

    std::wstring Uri::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }

    void Uri::WriteToEtw(uint16 contextSequenceId) const
    {
        switch (type_)
        {
        case UriType::AuthorityAbEmpty:
            CommonEventSource::Events->UriAuthorityTrace(
                contextSequenceId, 
                scheme_,
                authority_,
                path_.empty() ? (fragment_.empty() ? L"" : L"/") : path_,
                query_.empty() ? L"" : L"?",
                query_,
                fragment_.empty() ? L"" : L"#",
                fragment_);
            break;

        case UriType::Absolute:
            CommonEventSource::Events->UriTrace(
                contextSequenceId, 
                scheme_,
                path_.empty() ? L"/" : path_,
                query_.empty() ? L"" : L"?",
                query_,
                fragment_.empty() ? L"" : L"#",
                fragment_);
            break;

        case UriType::Rootless:
        case UriType::Empty:
            CommonEventSource::Events->UriTrace(
                contextSequenceId, 
                scheme_,
                path_,
                query_.empty() ? L"" : L"?",
                query_,
                fragment_.empty() ? L"" : L"#",
                fragment_);
            break;

        default:
            CommonEventSource::Events->UnrecognizedUriTrace(
                contextSequenceId, 
                static_cast<int>(type_),
                scheme_,
                path_,
                query_.empty() ? L"" : L"?",
                query_,
                fragment_.empty() ? L"" : L"#",
                fragment_);
            break;
        }
    }

    void Uri::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
    {
        switch (type_)
        {
        case UriType::AuthorityAbEmpty:
            w << scheme_ << L"://" << authority_;
            break;

        case UriType::Absolute:
        case UriType::Rootless:
        case UriType::Empty:
            w << scheme_ << L":";
            break;

        default:
            w << L"UNRECOGNIZED-URI-TYPE: '" << static_cast<int>(type_) << L"'" << scheme_ << L":";
        }

        if (!path_.empty())
        {
            w << path_;
        }
        else if (type_ == UriType::Absolute)
        {
            w << L"/";
        }
        else if (UriType::AuthorityAbEmpty == type_ && !fragment_.empty()) // for service group
        {
            w << L"/";
        }

        if (!query_.empty())
        {
            w << L"?" << query_;
        }
        if (!fragment_.empty())
        {
            w << L"#" << fragment_;
        }
    }

    bool Uri::TryParse(std::wstring const & input, __out Uri & output)
    {
        return TryParse(input, false, output);
    }

    bool Uri::TryParseAndTraceOnFailure(std::wstring const & input, __out Uri & output)
    {
        return TryParse(input, true, output);
    }

    bool Uri::TryParse(std::wstring const & input, bool traceOnFailure, __out Uri & output)
    {
        Uri::Parser parser(traceOnFailure);

        if (!parser.TryParse(input))
        {
            return false;
        }

        Uri uri;
        uri.type_ = parser.UriType;
        uri.scheme_ = std::wstring(parser.SchemeStart, parser.SchemeEnd);
        uri.authority_ = std::wstring(parser.AuthorityStart, parser.AuthorityEnd);
        uri.hostType_ = parser.HostType;
        uri.host_ = std::wstring(parser.HostStart, parser.HostEnd);
        uri.port_ = parser.Port;
        uri.path_ = std::wstring(parser.PathStart, parser.PathEnd);
        uri.query_ = std::wstring(parser.QueryStart, parser.QueryEnd);
        uri.fragment_ = std::wstring(parser.FragmentStart, parser.FragmentEnd);

        uri.Normalize();

        size_t segmentCount = parser.PathSegments.size();
        uri.pathSegments_.reserve(segmentCount);
        for (size_t i=0; i<segmentCount; i++)
        {
            std::wstring segment(parser.PathSegments[i].first, parser.PathSegments[i].second);

            // We disallow relative references, so we do not support '.' or '..' in the path.
            if (((segment.size() == 1) && (segment[0] == L'.')) ||
                ((segment.size() == 2) && (segment[0] == L'.') && (segment[1] == L'.')))
            {
                if (traceOnFailure)
                {
                    Trace.WriteNoise(
                        Uri::TraceCategory,
                        "Parsing Uri '{0}' failed because it includes '.' or '..'.  Relative paths are not supported.",
                        input);
                }

                return false;
            }

            uri.pathSegments_.push_back(std::move(segment));
        }

        // We disallow user information.  If we need to support user information, we cannot lowercase
        // the whole authority since user information is case-sensitive.
        if (parser.AuthorityStart != parser.HostStart)
        {
            if (traceOnFailure)
            {
                Trace.WriteNoise(
                    Uri::TraceCategory,
                    "Parsing Uri '{0}' failed because user information (user@ in authority) is not supported.",
                    input);
            }

            return false;
        }

        output = std::move(uri);
        return true;
    }

    void Uri::Normalize()
    {
        CharLowerBuff(const_cast<wchar_t*>(scheme_.c_str()), static_cast<DWORD>(scheme_.size()));
        CharLowerBuff(const_cast<wchar_t*>(authority_.c_str()), static_cast<DWORD>(authority_.size()));

        // Removing trailing slash from authority with no path.
        if ((type_ == UriType::AuthorityAbEmpty) && (path_.size() == 1) && (path_[0] == L'/'))
        {
            path_.clear();
        }
    }

    // This is only used for test constructors that pass in the path directly.
    std::wstring Uri::FixPathForConstructor(std::wstring const & path)
    {
        if (!path.empty() && (path[0] != L'/'))
        {
            return L"/" + path;
        }
        else
        {
            return path;
        }
    }

    std::vector<std::wstring> Uri::GetSegments(std::wstring const & path)
    {
        std::vector<std::wstring> segments;

        StringUtility::Split<std::wstring>(path, segments, L"/");

        // Path includes initial '/', so remove spurious initial segment unless it is the only one
        if ((segments.size() > 1) && (segments[0].size() == 0))
        {
            segments.erase(segments.begin());
        }

        return segments;
    }

    bool Uri::IsPrefixOf(Uri const & other) const
    {
        // If schemes don't match, this is not a prefix.
        if (!StringUtility::AreEqualCaseInsensitive(Scheme, other.Scheme))
        {
            return false;
        }

        // If authorities don't match, this a prefix iff this's authority is empty.
        if (!StringUtility::AreEqualCaseInsensitive(Authority, other.Authority))
        {
            return Authority.empty() && !other.Authority.empty();
        }

        // If this path is empty and other path is not, we are a prefix.
        // Because we use authority, there is always an empty segment, so
        // we need this special case.
        if (this->Path.empty() && !other.Path.empty())
        {
            return true;
        }

        // Ensure other.Segments is at least as large as this->Segments
        if (this->Segments.size() > other.Segments.size())
        {
            return false;
        }

        // Verify all of this's segments appear in other's segments
        for (size_t i = 0; i < Segments.size(); i++)
        {
            if (Segments[i] != other.Segments[i])
            {
                return false;
            }
        }

        // If scheme, authority, and all segments matched and other has more segments,
        // this is a prefix.
        if (Segments.size() < other.Segments.size())
        {
            return true;
        }

        // Otherwise this is only a prefix if this does not have a fragment and
        // other does.
        return this->Fragment.empty() && !other.Fragment.empty();
    }

    namespace UriType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case AuthorityAbEmpty: w << "AuthorityAbEmpty"; return;
            case Absolute: w << "Absolute"; return;
            case Rootless: w << "Rootless"; return;
            case Empty: w << "Empty"; return;
            default: w << "Unknown UriType value: " << static_cast<int>(e); return;
            }
        }
    }

    namespace UriHostType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case None: w << "None"; return;
            case IPv4: w << "IPv4"; return;
            case IPv6: w << "IPv6"; return;
            case RegName: w << "RegName"; return;
            default: w << "Unknown UriHostType value: " << static_cast<int>(e); return;
            }
        }
    }
}
