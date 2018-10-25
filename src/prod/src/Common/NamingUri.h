// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class NamingUri : public Common::Uri
    {
    public:
        // NamingUri constants. Keep these in the same file to avoid
        // static initialization order issues since some of these
        // constants are interdependent.
        //
        static Common::GlobalWString NameUriScheme;
        static Common::GlobalWString RootAuthority;
        static Common::GlobalWString RootNamingUriString;
        static Common::GlobalWString InvalidRootNamingUriString;
        static Common::GlobalWString NamingUriConcatenationReservedTokenDelimiter;
        static Common::GlobalWString ReservedTokenDelimiters;
        static Common::Global<NamingUri> RootNamingUri;
        static Common::GlobalWString SegmentDelimiter;
        static Common::GlobalWString HierarchicalNameDelimiter;

        struct Hasher
        {
            size_t operator() (NamingUri const & key) const
            {
                return (!key.HasQueryOrFragment ? key.GetHash() : key.GetTrimQueryAndFragmentHash());
            }
            bool operator() (NamingUri const & left, NamingUri const & right) const
            {
                if (!left.HasQueryOrFragment && !right.HasQueryOrFragment)
                {
                    return left == right;
                }
                else
                {
                    return left.GetTrimQueryAndFragment() == right.GetTrimQueryAndFragment();
                }
            }
        };

        NamingUri() : Uri(NameUriScheme, L"")
        {
        }

        explicit NamingUri(std::wstring const & path) : Uri(NameUriScheme, L"", path)
        {
            ASSERT_IFNOT(IsNamingUri(*this), "{0} does not make a valid NamingUri", *this);
        }

        // Returns actual name after stripping off query strings, etc.
        __declspec(property(get=get_Name)) std::wstring Name;
        std::wstring get_Name() const { return ToString(); }

        __declspec(property(get=get_IsRootNamingUri)) bool IsRootNamingUri;
        bool get_IsRootNamingUri() const { return *this == RootNamingUri; }

        NamingUri GetAuthorityName() const;

        NamingUri GetParentName() const;

        NamingUri GetTrimQueryAndFragmentName() const;

        size_t GetHash() const;

        size_t GetTrimQueryAndFragmentHash() const;

        bool TryCombine(std::wstring const & path, __out NamingUri &) const;

        // Validates that the name can be created or deleted in naming for user operations.
        // Names can't be root URI, or have reserved characters (?).
        // If validateNameFragment is true, the name should not have characters reserved for service groups (#).
        // The API takes the URI as well as the string representation of the URI, to optimize
        // the cases when the string is cached and should not be re-evaluated (see Naming StoreService operations).
        static ErrorCode ValidateName(NamingUri const & uri, std::wstring const & uriString, bool allowFragment);

        static NamingUri Combine(NamingUri const &, std::wstring const & path);

        bool IsPrefixOf(NamingUri const &) const;

        static HRESULT TryParse(
            FABRIC_URI name,
            std::wstring const & traceId,
            __out NamingUri & nameUri);

        static ErrorCode TryParse(
            FABRIC_URI name,
            Common::StringLiteral const & parameterName,
            __inout NamingUri & nameUri);

        static ErrorCode TryParse(
            std::wstring const & nameText,
            std::wstring const & traceId,
            __out NamingUri & nameUri);

        static bool TryParse(std::wstring const & nameText, __out NamingUri & nameUri);

        // wstring has an implicit ctor that will get bound to if FABRIC_URI is nullptr,
        // resulting in an AV. The TryParse() functions that take an additional traceId/parameterName
        // for debugging are preferred, but create this overload to avoid binding to the
        // TryParse(wstring const &, _out NamingUri const &) function above by accident.
        //
        static bool TryParse(FABRIC_URI name, __out NamingUri & nameUri);

        static bool IsNamingUri(Common::Uri const & question);

        //
        // Routines to convert fabric namespace names to an ID.
        // Fabric namespace names are of the form <scheme>:/<path>.
        // The scheme can be either 'fabric' or 'sys'. The name to ID conversion
        // removes the scheme portion and replaces / with ~.
        //

        static ErrorCode FabricNameToId(std::wstring const & name, __inout std::wstring & escapedId);
        static ErrorCode FabricNameToId(std::wstring const & name, bool useDelimiter, __inout std::wstring & escapedId);

        static ErrorCode IdToFabricName(std::wstring const& scheme, std::wstring const& id, __inout std::wstring &name);

        static ErrorCode EscapeString(std::wstring const & input, __inout std::wstring & output);
        static ErrorCode UnescapeString(std::wstring const & input, __inout std::wstring & output);

        // This method calls EscapeString internally
        // Escape string converts all non ascii characters into %xx representations.
        // However, this method is not actually URL standards compliant as it does not encode values such
        // as the comma or semi colon, which is deemed as reserved characters in a URL.
        //
        // This method encodes the following characters in addition to what EscapeString encodes:
        // dollar("$")
        // plus sign("+")
        // comma(",")
        // colon(":")
        // semi - colon(";")
        // equals("=")
        // 'At' symbol("@")
        //
        // The following characters are already encoded by EscapeString. 
        // We encode them again in case that implementation changes. This way, we also ensure that
        // this method encodes URLs safely without OS restrictions - see the forward slash for example.
        // ampersand ("&")
        // forward slash("/")
        // question mark("?")
        // pound("#")
        // less than and greater than("<>")
        // open and close brackets("[]")
        // open and close braces("{}")
        // pipe("|")
        // backslash("\")
        // caret("^")
        // percent("%")
        // space(" ")
        //
        // To unescape this, use UnescapeString. EscapeString will not encode the characters above, but
        // the matching function will decode.
        static ErrorCode UrlEscapeString(std::wstring const & input, __out std::wstring & output);

    private:
        NamingUri(
            std::wstring const & scheme,
            std::wstring const & authority,
            std::wstring const & path,
            std::wstring const & query,
            std::wstring const & fragment);
        static ErrorCode ParseUnsafeUri(std::wstring const & input, std::wstring & protocol, std::wstring & host, std::wstring & path, std::wstring & queryFragment);
        static void ParseHost(std::wstring & input, std::wstring & host, std::wstring & remain);
        static void ParsePath(std::wstring & input, std::wstring & path, std::wstring & remain);
    };
}
