// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class Uri::Parser
    {
    private:
        class UnicodeRange;

        static const UnicodeRange DiacriticalMarksRange;
        static const UnicodeRange DiacriticalMarksSupplementRange;
        static const UnicodeRange HalfMarksRange;
        static const UnicodeRange DiacriticalMarksForSymbolsRange;
        static const UnicodeRange NonAsciiRange;

    public:
        typedef std::wstring::const_iterator iterator;

        __declspec(property(get=get_UriType)) UriType::Enum UriType;
        __declspec(property(get=get_SchemeStart)) iterator SchemeStart;
        __declspec(property(get=get_SchemeEnd)) iterator SchemeEnd;
        __declspec(property(get=get_AuthorityStart)) iterator AuthorityStart;
        __declspec(property(get=get_AuthorityEnd)) iterator AuthorityEnd;
        __declspec(property(get=get_HostType)) UriHostType::Enum HostType;
        __declspec(property(get=get_HostStart)) iterator HostStart;
        __declspec(property(get=get_HostEnd)) iterator HostEnd;
        __declspec(property(get=get_Port)) int Port;
        __declspec(property(get=get_PathStart)) iterator PathStart;
        __declspec(property(get=get_PathEnd)) iterator PathEnd;
        __declspec(property(get=get_QueryStart)) iterator QueryStart;
        __declspec(property(get=get_QueryEnd)) iterator QueryEnd;
        __declspec(property(get=get_FragmentStart)) iterator FragmentStart;
        __declspec(property(get=get_FragmentEnd)) iterator FragmentEnd;
        __declspec(property(get=get_PathSegments)) std::vector<std::pair<iterator,iterator>> & PathSegments;

        Parser(bool traceOnFailure);

        UriType::Enum get_UriType() { return type_; }
        iterator get_SchemeStart() { return schemeStart_; }
        iterator get_SchemeEnd() { return schemeEnd_; }
        iterator get_AuthorityStart() { return authorityStart_; }
        iterator get_AuthorityEnd() { return authorityEnd_; }
        UriHostType::Enum get_HostType() { return hostType_; }
        iterator get_HostStart() { return hostStart_; }
        iterator get_HostEnd() { return hostEnd_; }
        int get_Port() { return port_; }
        iterator get_PathStart() { return pathStart_; }
        iterator get_PathEnd() { return pathEnd_; }
        iterator get_QueryStart() { return queryStart_; }
        iterator get_QueryEnd() { return queryEnd_; }
        iterator get_FragmentStart() { return fragmentStart_; }
        iterator get_FragmentEnd() { return fragmentEnd_; }
        std::vector<std::pair<iterator,iterator>> & get_PathSegments() { return pathSegments_; }

        bool TryParse(std::wstring const & input);

    private:
        bool Fail(iterator position, wchar_t const * tag);

        // Match methods return true if the string starting at the first paramter matches the
        // specified element.  If they return true, 'end' points at the next character after
        // the element, otherwise 'end' is unmodified.
        //
        // These methods are also all aware of the end of the input string and do not read
        // beyond it.
        bool MatchUri(iterator cursor, __out iterator & end);
        bool MatchScheme(iterator start, __out iterator & end);
        bool MatchHierPart(iterator start, __out iterator & end);
        bool MatchAuthority(iterator start, __out iterator & end);
        bool MatchUserInfo(iterator cursor, __out iterator & end);
        bool MatchHost(iterator start, __out iterator & end);
        bool MatchPort(iterator cursor, __out iterator & end);
        bool MatchIPLiteral(iterator cursor, __out iterator & end);
        bool MatchIPv6Address(iterator start, __out iterator & end);
        bool MatchH16ColonH16N(iterator cursor, int maxSepCount, __out iterator & end);
        bool MatchH16ColonN(iterator cursor, int count, __out iterator & end);
        bool MatchH16(iterator cursor, __out iterator & end);
        bool MatchLs32(iterator start, __out iterator & end);
        bool MatchIPv4Address(iterator cursor, __out iterator & end);
        bool MatchDecOctet(iterator start, __out iterator & end);
        bool MatchRegName(iterator cursor, __out iterator & end);
        bool MatchPathAbEmpty(iterator start, __out iterator & end);
        bool MatchPathAbsolute(iterator start, __out iterator & end);
        bool MatchPathRootless(iterator start, __out iterator & end);
        bool MatchPathEmpty(iterator start, __out iterator & end);
        bool MatchSegment(iterator start, __out iterator & end);
        bool MatchSegmentNz(iterator start, __out iterator & end);
        bool MatchPChar(iterator start, __out iterator & end);
        bool MatchQueryOrFragment(iterator cursor, __out iterator & end);
        bool MatchQuery(iterator start, __out iterator & end);
        bool MatchFragment(iterator start, __out iterator & end);
        bool MatchPctEncoded(iterator start, __out iterator & end);
        bool MatchUnreserved(iterator start, __out iterator & end);
        bool MatchReserved(iterator start, __out iterator & end);
        bool MatchGenDelims(iterator start, __out iterator & end);
        bool MatchSubDelims(iterator start, __out iterator & end);
        bool MatchWhitespace(iterator start, __out iterator & end);
        bool MatchAlpha(iterator start, __out iterator & end);
        bool MatchLiteral(iterator cursor, wchar_t const * literal, __out iterator & end);
        bool MatchChar(iterator start, wchar_t ch, __out iterator & end);
        bool MatchChar(iterator start, LPCWSTR chars, __out iterator & end);

        bool IsSurrogatePair(wchar_t leading, wchar_t trailing);
        bool IsDiacriticalMark(wchar_t ch);
        bool IsDiacriticalMarkSupplement(wchar_t ch);
        bool IsHalfMark(wchar_t ch);
        bool IsDiacriticalMarkForSymbols(wchar_t ch);
        bool IsNonAscii(wchar_t ch);

        iterator InitializePath(UriType::Enum type, iterator pathStart, iterator pathEnd);

        UriType::Enum type_;
        iterator inputStart_;
        iterator inputEnd_;
        iterator schemeStart_;
        iterator schemeEnd_;
        iterator authorityStart_;
        iterator authorityEnd_;
        UriHostType::Enum hostType_;
        iterator hostStart_;
        iterator hostEnd_;
        int port_;
        iterator pathStart_;
        iterator pathEnd_;
        iterator queryStart_;
        iterator queryEnd_;
        iterator fragmentStart_;
        iterator fragmentEnd_;
        std::vector<std::pair<iterator,iterator>> pathSegments_;

        bool traceOnFailure_;

        static LPCWSTR hexDigit_;
        static LPCWSTR decDigit_;
    };
}
