// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    class Uri::Parser::UnicodeRange
    {
    public:
        UnicodeRange(wchar_t first, wchar_t last) : first_(first), last_(last) { }

        bool Contains(wchar_t ch) const { return (first_ <= ch && ch <= last_); }

    private:
        wchar_t first_;
        wchar_t last_;
    };

    // Ranges taken from version 6.1 of the character code charts (http://www.unicode.org/charts/)
    //
    const Uri::Parser::UnicodeRange Uri::Parser::DiacriticalMarksRange(0x0300, 0x036F);
    const Uri::Parser::UnicodeRange Uri::Parser::DiacriticalMarksSupplementRange(0x1DC0, 0x1DFF);
    const Uri::Parser::UnicodeRange Uri::Parser::HalfMarksRange(0xFE20, 0xFE2F);
    const Uri::Parser::UnicodeRange Uri::Parser::DiacriticalMarksForSymbolsRange(0x20D0, 0x20FF);

    // IsCharAlpha already accepts many unicode ranges for non-English characters, but some
    // ranges are not allowed for unknown reasons (e.g. Hiragana). The main purpose of the
    // IsCharAlpha check is to verify against disallowed ASCII characters, so to support
    // non-English characters, we allow everything past the extended ASCII range.
    //
    // Surrogate pair characters start at 0xD800.
    //
    const Uri::Parser::UnicodeRange Uri::Parser::NonAsciiRange(0x00FF, 0xD7FF);

    LPCWSTR Uri::Parser::hexDigit_ = L"0123456789abcdefABCDEF";
    LPCWSTR Uri::Parser::decDigit_ = L"0123456789";

    // RFC 3986                   URI Generic Syntax               January 2005
    //                   http://www.ietf.org/rfc/rfc3986.txt
    //
    // Appendix A.  Collected ABNF for URI
    //
    //    URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    //
    //    hier-part     = "//" authority path-abempty
    //                  / path-absolute
    //                  / path-rootless
    //                  / path-empty
    //
    // *  URI-reference = URI / relative-ref
    //
    //    absolute-URI  = scheme ":" hier-part [ "?" query ]
    //
    // *  relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
    //
    // *  relative-part = "//" authority path-abempty
    //                  / path-absolute
    //                  / path-noscheme
    //                  / path-empty
    //
    //    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    //
    //    authority     = [ userinfo "@" ] host [ ":" port ]
    //    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
    //    host          = IP-literal / IPv4address / reg-name
    //    port          = *DIGIT
    //
    //    IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
    //
    // +  IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    //
    //    IPv6address   =                            6( h16 ":" ) ls32
    //                  /                       "::" 5( h16 ":" ) ls32
    //                  / [               h16 ] "::" 4( h16 ":" ) ls32
    //                  / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
    //                  / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
    //                  / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
    //                  / [ *4( h16 ":" ) h16 ] "::"              ls32
    //                  / [ *5( h16 ":" ) h16 ] "::"              h16
    //                  / [ *6( h16 ":" ) h16 ] "::"
    //
    //    h16           = 1*4HEXDIG
    //    ls32          = ( h16 ":" h16 ) / IPv4address
    //    IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
    //
    //    dec-octet     = DIGIT                 ; 0-9
    //                  / %x31-39 DIGIT         ; 10-99
    //                  / "1" 2DIGIT            ; 100-199
    //                  / "2" %x30-34 DIGIT     ; 200-249
    //                  / "25" %x30-35          ; 250-255
    //
    //    reg-name      = *( unreserved / pct-encoded / sub-delims )
    //
    // *  path          = path-abempty    ; begins with "/" or is empty
    //                  / path-absolute   ; begins with "/" but not "//"
    //                  / path-noscheme   ; begins with a non-colon segment
    //                  / path-rootless   ; begins with a segment
    //                  / path-empty      ; zero characters
    //
    //    path-abempty  = *( "/" segment )
    //    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    // *  path-noscheme = segment-nz-nc *( "/" segment )
    //    path-rootless = segment-nz *( "/" segment )
    //    path-empty    = 0<pchar>
    //
    //    segment       = *pchar
    //    segment-nz    = 1*pchar
    // *  segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
    //                  ; non-zero-length segment without any colon ":"
    //
    //    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    //
    //    query         = *( pchar / "/" / "?" )
    //
    //    fragment      = *( pchar / "/" / "?" )
    //
    //    pct-encoded   = "%" HEXDIG HEXDIG
    //
    //    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    //    reserved      = gen-delims / sub-delims
    //    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    //    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
    //                  / "*" / "+" / "," / ";" / "=" / "{" / "}"
    //
    // (*) Items marked with asterisk are only required for URI-reference.  We do not support
    //     relative references, so these are not implemented.
    // (+) IPvFuture is not supported and not implemented.
    //

    LPCWSTR UnreservedChars = L"-._~";
    LPCWSTR GenDelimsChars = L":/?#[]@";
    LPCWSTR SubDelimsChars = L"!$&'()*+,;={}";
    LPCWSTR WhitespaceChars = L" \t\r\n";

    Uri::Parser::Parser(bool traceOnFailure)
        : traceOnFailure_(traceOnFailure)
    {
    }

    bool Uri::Parser::TryParse(std::wstring const & input)
    {
        type_ = UriType::Empty;
        inputStart_ = input.begin();
        inputEnd_ = input.end();
        schemeStart_ = input.end();
        schemeEnd_ = input.end();
        authorityStart_ = input.end();
        authorityEnd_ = input.end();
        hostType_ = UriHostType::None;
        hostStart_ = input.end();
        hostEnd_ = input.end();
        port_ = -1;
        pathStart_ = input.end();
        pathEnd_ = input.end();
        queryStart_ = input.end();
        queryEnd_ = input.end();
        fragmentStart_ = input.end();
        fragmentEnd_ = input.end();
        pathSegments_.clear();

        iterator cursor;

        // System.Uri compatibility: strip leading and trailing whitespace.
        // We need to move inputEnd_ before we start parsing since System.Uri compatibility
        // also allows whitespace in the Uri and we don't want to bind trailing whitespace.
        while ((inputStart_ < inputEnd_) && MatchWhitespace(inputStart_, /*out*/inputStart_))
        {
        }
        while ((inputStart_ < inputEnd_) && MatchWhitespace(inputEnd_ - 1, /*out*/cursor))
        {
            --inputEnd_;
        }

        if (!MatchUri(inputStart_, /*out*/cursor)) { return false; }
        if (cursor != inputEnd_) { return Fail(cursor, L"Invalid character."); }

        return true;
    }

    bool Uri::Parser::Fail(iterator position, wchar_t const * tag)
    {
        if (!traceOnFailure_)
        {
            return false;
        }

        auto c = ((position < inputEnd_) ? *position : L' ');
        Trace.WriteInfo(
            Uri::TraceCategory,
            "Parsing Uri '{0}' failed at character {1}('{2}')(U+{3:X}): {4}",
            std::wstring(inputStart_, inputEnd_),
            position - inputStart_,
            c,
            static_cast<LONG>(c),
            tag);

        return false;
    }

    //    URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    bool Uri::Parser::MatchUri(iterator cursor, __out iterator & end)
    {
        if (!MatchScheme(cursor, /*out*/cursor)) { return false; }
        if (!MatchChar(cursor, L':', /*out*/cursor)) { return Fail(cursor, L"Scheme must end with ':'."); }
        if (!MatchHierPart(cursor, /*out*/cursor)) { return false; }

        iterator query;
        if (MatchChar(cursor, L'?', /*out*/query)) { MatchQuery(query, /*out*/cursor); }

        iterator fragment;
        if (MatchChar(cursor, L'#', /*out*/fragment)) { MatchFragment(fragment, /*out*/cursor); }

        end = cursor;
        return true;
    }

    //    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    bool Uri::Parser::MatchScheme(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        if (!MatchAlpha(start, /*out*/cursor)) { return Fail(cursor, L"Scheme must start with ALPHA."); }

        while (MatchAlpha(cursor, /*out*/cursor) ||
               MatchChar(cursor, decDigit_, /*out*/cursor) ||
               MatchChar(cursor, L"+-.", /*out*/cursor))
        {
        }

        schemeStart_ = start;
        schemeEnd_ = cursor;

        end = cursor;
        return true;
    }

    //    hier-part     = "//" authority path-abempty
    //                  / path-absolute
    //                  / path-rootless
    //                  / path-empty
    bool Uri::Parser::MatchHierPart(iterator start, __out iterator & end)
    {
        iterator cursor = start;

        if (MatchLiteral(start, L"//", /*out*/cursor))
        {
            if (MatchAuthority(cursor, /*out*/cursor) && MatchPathAbEmpty(cursor, /*out*/end)) { return true; }
        }
        else if (MatchPathAbsolute(start, /*out*/end)) { return true; }
        else if (MatchPathRootless(start, /*out*/end)) { return true; }
        else if (MatchPathEmpty(start, /*out*/end)) { return true; }

        return Fail(start, L"Invalid path.");
    }

    //    authority     = [ userinfo "@" ] host [ ":" port ]
    bool Uri::Parser::MatchAuthority(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        iterator atsign;
        if (MatchUserInfo(cursor, /*out*/atsign))
        {
            MatchChar(atsign, L'@', /*out*/cursor);
        }

        if (!MatchHost(cursor, /*out*/cursor)) { return false; }

        iterator port;
        if (MatchChar(cursor, L':', /*out*/port))
        {
            MatchPort(port, /*out*/cursor);
        }

        authorityStart_ = start;
        authorityEnd_ = cursor;

        end = cursor;
        return true;
    }

    //    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
    bool Uri::Parser::MatchUserInfo(iterator cursor, __out iterator & end)
    {
        while (MatchUnreserved(cursor, /*out*/cursor) ||
               MatchPctEncoded(cursor, /*out*/cursor) ||
               MatchSubDelims(cursor, /*out*/cursor) ||
               MatchChar(cursor, L':', /*out*/cursor))
        {
        }

        end = cursor;
        return true;
    }

    //    host          = IP-literal / IPv4address / reg-name
    bool Uri::Parser::MatchHost(iterator start, __out iterator & end)
    {
        if (!MatchIPLiteral(start, /*out*/end) &&
            !MatchIPv4Address(start, /*out*/end) &&
            !MatchRegName(start, /*out*/end))
        {
            hostType_ = UriHostType::None;
            return Fail(start, L"Invalid host.");
        }

        hostStart_ = start;
        hostEnd_ = end;
        return true;
    }

    //    port          = *DIGIT
    bool Uri::Parser::MatchPort(iterator cursor, __out iterator & end)
    {
        int port = 0;
        iterator next;
        while (MatchChar(cursor, decDigit_, /*out*/next))
        {
            port *= 10;
            port += static_cast<int>(*cursor - L'0');
            cursor = next;

            if (port >= 0x10000) { return false; }
        }

        port_ = port;
        end = cursor;
        return true;
    }

    //    IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
    bool Uri::Parser::MatchIPLiteral(iterator cursor, __out iterator & end)
    {
        if (!MatchChar(cursor, L'[', /*out*/cursor)) { return false; }
        if (!MatchIPv6Address(cursor, /*out*/cursor)) { return Fail(cursor, L"Invalid IPv6 address"); }
        return MatchChar(cursor, L']', /*out*/end);
    }

    //    IPv6address   =                            6( h16 ":" ) ls32
    //                  /                       "::" 5( h16 ":" ) ls32
    //                  / [               h16 ] "::" 4( h16 ":" ) ls32
    //                  / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
    //                  / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
    //                  / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
    //                  / [ *4( h16 ":" ) h16 ] "::"              ls32
    //                  / [ *5( h16 ":" ) h16 ] "::"              h16
    //                  / [ *6( h16 ":" ) h16 ] "::"
    bool Uri::Parser::MatchIPv6Address(iterator start, __out iterator & end)
    {
        iterator cursor;

        //    IPv6address   =                            6( h16 ":" ) ls32
        if (MatchH16ColonN(start, 6, /*out*/cursor) &&
            MatchLs32(cursor, /*out*/end))
        {
            hostType_ = UriHostType::IPv6;
            return true;
        }

        //                  /                       "::" 5( h16 ":" ) ls32
        if (MatchLiteral(start, L"::", /*out*/cursor) &&
            MatchH16ColonN(cursor, 5, /*out*/cursor) &&
            MatchLs32(cursor, /*out*/end))
        {
            hostType_ = UriHostType::IPv6;
            return true;
        }

        //                  / [               h16 ] "::" 4( h16 ":" ) ls32
        //                  / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
        //                  / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
        //                  / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
        //                  / [ *4( h16 ":" ) h16 ] "::"              ls32
        int limit = 4;
        for (int i=0; i<=limit; i++)
        {
            if (MatchH16ColonH16N(start, i, /*out*/cursor) &&
                MatchLiteral(cursor, L"::", /*out*/cursor) &&
                MatchH16ColonN(cursor, limit-i, /*out*/cursor) &&
                MatchLs32(cursor, /*out*/end))
            {
                hostType_ = UriHostType::IPv6;
                return true;
            }
        }

        //                  / [ *5( h16 ":" ) h16 ] "::"              h16
        if (MatchH16ColonH16N(start, 5, /*out*/cursor) &&
            MatchLiteral(cursor, L"::", /*out*/cursor) &&
            MatchH16(cursor, /*out*/end))
        {
            hostType_ = UriHostType::IPv6;
            return true;
        }

        //                  / [ *6( h16 ":" ) h16 ] "::"
        if (MatchH16ColonH16N(start, 6, /*out*/cursor) &&
            MatchLiteral(cursor, L"::", /*out*/end))
        {
            hostType_ = UriHostType::IPv6;
            return true;
        }

        return false;
    }

    bool Uri::Parser::MatchH16ColonH16N(iterator cursor, int maxSepCount, __out iterator & end)
    {
        if (!MatchH16(cursor, /*out*/cursor)) { return false; }

        for (int i=0; i<maxSepCount; i++)
        {
            iterator h16;
            if (!MatchChar(cursor, L':', /*out*/h16) || !MatchH16(h16, /*out*/cursor))
            {
                end = cursor;
                return true;
            }
        }

        end = cursor;
        return true;
    }

    bool Uri::Parser::MatchH16ColonN(iterator cursor, int count, __out iterator & end)
    {
        for (int i=0; i<count; i++)
        {
            iterator colon;
            if (!MatchH16(cursor, /*out*/colon) || !MatchChar(colon, L':', /*out*/cursor)) { return false; }
        }

        end = cursor;
        return true;
    }

    //    h16           = 1*4HEXDIG
    bool Uri::Parser::MatchH16(iterator cursor, __out iterator & end)
    {
        if (!MatchChar(cursor, hexDigit_, /*out*/cursor)) { return false; }

        for (int i=0; i<3; i++)
        {
            if (!MatchChar(cursor, hexDigit_, /*out*/cursor)) { break; }
        }

        end = cursor;
        return true;
    }

    //    ls32          = ( h16 ":" h16 ) / IPv4address
    bool Uri::Parser::MatchLs32(iterator start, __out iterator & end)
    {
        iterator cursor;
        if (MatchH16(start, /*out*/cursor) &&
            MatchChar(cursor, L':', /*out*/cursor) &&
            MatchH16(cursor, /*out*/end))
        {
            return true;
        }
        else
        {
            return MatchIPv4Address(start, /*out*/end);
        }
    }

    //    IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
    bool Uri::Parser::MatchIPv4Address(iterator cursor, __out iterator & end)
    {
        for (int i=0; i<3; i++)
        {
            if (!MatchDecOctet(cursor, /*out*/cursor)) { return false; }
            if (!MatchChar(cursor, L'.', /*out*/cursor)) { return false; }
        }

        if (MatchDecOctet(cursor, /*out*/end))
        {
            hostType_ = UriHostType::IPv4;
            return true;
        }
        else { return false; }
    }

    //    dec-octet     = DIGIT                 ; 0-9
    //                  / %x31-39 DIGIT         ; 10-99
    //                  / "1" 2DIGIT            ; 100-199
    //                  / "2" %x30-34 DIGIT     ; 200-249
    //                  / "25" %x30-35          ; 250-255
    bool Uri::Parser::MatchDecOctet(iterator start, __out iterator & end)
    {
        iterator cursor;

        // Go in reverse order to match greedily
        if (MatchLiteral(start, L"25", /*out*/cursor) &&
            MatchChar(cursor, L"012345", /*out*/end))
        {
            return true;
        }
        else if (MatchChar(start, L'2', /*out*/cursor) &&
                 MatchChar(cursor, L"01234", /*out*/cursor) &&
                 MatchChar(cursor, decDigit_, /*out*/end))
        {
            return true;
        }
        else if (MatchChar(start, L'1', /*out*/cursor) &&
                 MatchChar(cursor, decDigit_, /*out*/cursor) &&
                 MatchChar(cursor, decDigit_, /*out*/end))
        {
            return true;
        }
        else if (MatchChar(start, L"123456789", /*out*/cursor) &&
                 MatchChar(cursor, decDigit_, /*out*/end))
        {
            return true;
        }
        else
        {
            return MatchChar(start, decDigit_, /*out*/end);
        }
    }

    //    reg-name      = *( unreserved / pct-encoded / sub-delims )
    bool Uri::Parser::MatchRegName(iterator cursor, __out iterator & end)
    {
        while (MatchUnreserved(cursor, /*out*/cursor) ||
               MatchPctEncoded(cursor, /*out*/cursor) ||
               MatchSubDelims(cursor, /*out*/cursor))
        {
        }

        hostType_ = UriHostType::RegName;
        end = cursor;
        return true;
    }

    //    path-abempty  = *( "/" segment )
    bool Uri::Parser::MatchPathAbEmpty(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        iterator segment;
        while (MatchChar(cursor, L'/', /*out*/segment) && MatchSegment(segment, /*out*/cursor))
        {
        }

        end = InitializePath(UriType::AuthorityAbEmpty, start, cursor);
        return true;
    }

    //    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    bool Uri::Parser::MatchPathAbsolute(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        if (!MatchChar(cursor, L'/', /*out*/cursor)) { return false; }

        if (MatchSegmentNz(cursor, /*out*/cursor))
        {
            iterator segment;
            while (MatchChar(cursor, L'/', /*out*/segment) && MatchSegment(segment, /*out*/cursor)) { }
        }

        end = InitializePath(UriType::Absolute, start, cursor);
        return true;
    }

    //    path-rootless = segment-nz *( "/" segment )
    bool Uri::Parser::MatchPathRootless(iterator start, __out iterator & end)
    {
        iterator cursor = start;

        if (!MatchSegmentNz(cursor, /*out*/cursor)) { return false; }

        iterator segment;
        while (MatchChar(cursor, L'/', /*out*/segment) && MatchSegment(segment, /*out*/cursor)) { }

        end = InitializePath(UriType::Rootless, start, cursor);
        return true;
    }

    //    path-empty    = 0<pchar>
    bool Uri::Parser::MatchPathEmpty(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        if (MatchPChar(cursor, /*out*/cursor)) { return false; }

        end = InitializePath(UriType::Empty, start, start);
        return true;
    }

    //    segment       = *pchar
    bool Uri::Parser::MatchSegment(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        while (MatchPChar(cursor, /*out*/cursor)) { }

        end = cursor;
        pathSegments_.push_back(std::pair<iterator,iterator>(start, end));
        return true;
    }

    //    segment-nz    = 1*pchar
    bool Uri::Parser::MatchSegmentNz(iterator start, __out iterator & end)
    {
        iterator cursor = start;
        if (!MatchPChar(start, /*out*/cursor)) { return false; }

        while (MatchPChar(cursor, /*out*/cursor)) { }

        end = cursor;
        pathSegments_.push_back(std::pair<iterator,iterator>(start, end));
        return true;
    }

    //    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    bool Uri::Parser::MatchPChar(iterator start, __out iterator & end)
    {
        // System.Uri allows inner spaces in the path, so we do too.
        return (MatchUnreserved(start, /*out*/end) ||
                MatchPctEncoded(start, /*out*/end) ||
                MatchSubDelims(start, /*out*/end) ||
                MatchChar(start, L":@", /*out*/end) ||
                MatchWhitespace(start, /*out*/end));
    }

    //    query         = *( pchar / "/" / "?" )
    //    fragment      = *( pchar / "/" / "?" )
    bool Uri::Parser::MatchQueryOrFragment(iterator cursor, __out iterator & end)
    {
        while (MatchPChar(cursor, /*out*/cursor) ||
               MatchChar(cursor, L"/?", /*out*/cursor))
        {
        }

        end = cursor;
        return true;
    }

    bool Uri::Parser::MatchQuery(iterator start, __out iterator & end)
    {
        if (!MatchQueryOrFragment(start, /*out*/end)) { return false; }

        queryStart_ = start;
        queryEnd_ = end;
        return true;
    }

    bool Uri::Parser::MatchFragment(iterator start, __out iterator & end)
    {
        if (!MatchQueryOrFragment(start, /*out*/end)) { return false; }

        fragmentStart_ = start;
        fragmentEnd_ = end;
        return true;
    }

    //    pct-encoded   = "%" HEXDIG HEXDIG
    bool Uri::Parser::MatchPctEncoded(iterator start, __out iterator & end)
    {
        iterator cursor;
        return (MatchChar(start, L'%', /*out*/cursor) &&
                MatchChar(cursor, hexDigit_, /*out*/cursor) &&
                MatchChar(cursor, hexDigit_, /*out*/end));
    }

    //    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    bool Uri::Parser::MatchUnreserved(iterator start, __out iterator & end)
    {
        return (MatchAlpha(start, /*out*/end) ||
                MatchChar(start, decDigit_, /*out*/end) ||
                MatchChar(start, UnreservedChars, /*out*/end));
    }

    //    reserved      = gen-delims / sub-delims
    bool Uri::Parser::MatchReserved(iterator start, __out iterator & end)
    {
        return (MatchGenDelims(start, /*out*/end) ||
                MatchSubDelims(start, /*out*/end));
    }

    //    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    bool Uri::Parser::MatchGenDelims(iterator start, __out iterator & end)
    {
        return MatchChar(start, GenDelimsChars, /*out*/end);
    }

    //    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
    //                  / "*" / "+" / "," / ";" / "=" / "{" / "}"
    bool Uri::Parser::MatchSubDelims(iterator start, __out iterator & end)
    {
        return MatchChar(start, SubDelimsChars, /*out*/end);
    }

    // Matches exactly one Whitespace character.
    bool Uri::Parser::MatchWhitespace(iterator start, __out iterator & end)
    {
        return MatchChar(start, WhitespaceChars, /*out*/end);
    }

    // Matches exactly one ALPHA character.
    bool Uri::Parser::MatchAlpha(iterator start, __out iterator & end)
    {
        if (start == inputEnd_) { return false; }

        if (IsCharAlpha(*start) ||
            IsDiacriticalMark(*start) ||
            IsDiacriticalMarkSupplement(*start) ||
            IsHalfMark(*start) ||
            IsDiacriticalMarkForSymbols(*start) ||
            IsNonAscii(*start))
        {
            end = start + 1;
            return true;
        }

        iterator const & next = (start + 1);

        if (next == inputEnd_) { return false; }

        if (IsSurrogatePair(*start, *next) || IsSurrogatePair(*next, *start))
        {
            end = next + 1;
            return true;
        }

        return false;
    }

    // Matches a literal string.
    bool Uri::Parser::MatchLiteral(iterator cursor, wchar_t const * literal, __out iterator & end)
    {
        for (size_t i=0; literal[i]; i++)
        {
            if (!MatchChar(cursor, literal[i], /*out*/cursor)) { return false; }
        }

        end = cursor;
        return true;
    }

    // Matches a single specific character
    bool Uri::Parser::MatchChar(iterator start, wchar_t ch, __out iterator & end)
    {
        if ((start == inputEnd_) || (*start != ch)) { return false; }

        end = start + 1;
        return true;
    }

    // Matches a single character that appears in the 'chars' string.
    bool Uri::Parser::MatchChar(iterator start, LPCWSTR chars, __out iterator & end)
    {
        if (start < inputEnd_)
        {
            for (size_t i=0; chars[i] != 0; i++)
            {
                if (*start == chars[i])
                {
                    end = start + 1;
                    return true;
                }
            }
        }

        return false;
    }

    bool Uri::Parser::IsSurrogatePair(wchar_t leading, wchar_t trailing)
    {
        return (IS_SURROGATE_PAIR(leading, trailing) == TRUE);
    }

    bool Uri::Parser::IsDiacriticalMark(wchar_t ch)
    {
        return DiacriticalMarksRange.Contains(ch);
    }

    bool Uri::Parser::IsDiacriticalMarkSupplement(wchar_t ch)
    {
        return DiacriticalMarksSupplementRange.Contains(ch);
    }

    bool Uri::Parser::IsHalfMark(wchar_t ch)
    {
        return HalfMarksRange.Contains(ch);
    }

    bool Uri::Parser::IsDiacriticalMarkForSymbols(wchar_t ch)
    {
        return DiacriticalMarksForSymbolsRange.Contains(ch);
    }

    bool Uri::Parser::IsNonAscii(wchar_t ch)
    {
        return NonAsciiRange.Contains(ch);
    }

    Uri::Parser::iterator Uri::Parser::InitializePath(UriType::Enum type, iterator pathStart, iterator pathEnd)
    {
        type_ = type;
        pathStart_ = pathStart;
        pathEnd_ = pathEnd;

        return pathEnd;
    }
}
