// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    GlobalWString NamingUri::NameUriScheme = make_global<std::wstring>(L"fabric");
    GlobalWString NamingUri::RootAuthority = make_global<std::wstring>(L"");
    GlobalWString NamingUri::RootNamingUriString = make_global<std::wstring>(L"fabric:");
    GlobalWString NamingUri::InvalidRootNamingUriString = make_global<std::wstring>(L"fabric://");
    GlobalWString NamingUri::NamingUriConcatenationReservedTokenDelimiter = make_global<std::wstring>(L"+");
    // All reserved token delimiters should be listed here.
    GlobalWString NamingUri::ReservedTokenDelimiters = make_global<std::wstring>(L"$+|~");
    Global<NamingUri> NamingUri::RootNamingUri = make_global<NamingUri>(L"");
    GlobalWString NamingUri::SegmentDelimiter = make_global<std::wstring>(L"/");
    GlobalWString NamingUri::HierarchicalNameDelimiter = make_global<std::wstring>(L"~");

    NamingUri::NamingUri(
        std::wstring const & scheme,
        std::wstring const & authority,
        std::wstring const & path,
        std::wstring const & query,
        std::wstring const & fragment)
        : Uri(scheme, authority, path, query, fragment)
    {
        ASSERT_IFNOT(IsNamingUri(*this), "Invalid naming uri");
    }

    NamingUri NamingUri::GetAuthorityName() const
    {
        return Segments.empty() ? NamingUri() : NamingUri(Segments[0]);
    }

    NamingUri NamingUri::GetParentName() const
    {
        if (Path.empty())
        {
            return NamingUri::RootNamingUri;
        }

        size_t index = Path.find_last_of(L"/");
        ASSERT_IF(index == wstring::npos, "No / in path?");

        wstring truncPath = Path.substr(0, index);
        return NamingUri(truncPath);
    }

    NamingUri NamingUri::GetTrimQueryAndFragmentName() const
    {
        if (this->Query.empty() && this->Fragment.empty())
        {
            return *this;
        }
        else
        {
            return NamingUri(this->Path);
        }
    }

    size_t NamingUri::GetHash() const
    {
        return StringUtility::GetHash(this->ToString());
    }

    size_t NamingUri::GetTrimQueryAndFragmentHash() const
    {
        return StringUtility::GetHash(this->GetTrimQueryAndFragment().ToString());
    }

    bool NamingUri::IsPrefixOf(NamingUri const & other) const
    {
        return Uri::IsPrefixOf(other);
    }

    bool NamingUri::TryParse(std::wstring const & input, __out NamingUri & output)
    {
        if (StringUtility::StartsWith<std::wstring>(input, InvalidRootNamingUriString))
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                "NamingUri '{0}' is invalid: authorities are not supported.",
                input);
            return false;
        }

        NamingUri uri;
        if (Uri::TryParse(input, uri) && IsNamingUri(uri))
        {
            output = std::move(uri);
            return true;
        }
        return false;
    }

    bool NamingUri::TryParse(FABRIC_URI name, __out NamingUri & output)
    {
        if (name == NULL)
        {
            Trace.WriteWarning(Uri::TraceCategory, "Invalid NULL parameter: FABRIC_URI.");
            return false;
        }

        std::wstring tempName(name);
        return NamingUri::TryParse(tempName, /*out*/output);
    }

    HRESULT NamingUri::TryParse(
        FABRIC_URI name,
        std::wstring const & traceId,
        __out NamingUri & nameUri)
    {
        // Validate and parse the input name pointer
        if (name == NULL)
        {
            Trace.WriteWarning(Uri::TraceCategory, traceId, "Invalid NULL parameter: name.");
            return E_POINTER;
        }

        auto error = ParameterValidator::IsValid(
            name,
            ParameterValidator::MinStringSize,
            CommonConfig::GetConfig().MaxNamingUriLength);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                traceId,
                "Input uri doesn't respect parameter size limits ({0}-{1}).",
                ParameterValidator::MinStringSize,
                CommonConfig::GetConfig().MaxNamingUriLength);
            return error.ToHResult();
        }

        std::wstring tempName(name);
        if (!NamingUri::TryParse(tempName, /*out*/nameUri))
        {
            Trace.WriteWarning(Uri::TraceCategory, traceId, "{0}: Input uri is not a valid naming uri.", tempName);
            return FABRIC_E_INVALID_NAME_URI;
        }

        return S_OK;
    }

    ErrorCode NamingUri::TryParse(
        FABRIC_URI name,
        StringLiteral const & parameterName,
        __inout NamingUri & nameUri)
    {
        // Validate and parse the input name pointer
        if (name == NULL)
        {
            ErrorCode innerError(ErrorCodeValue::ArgumentNull, wformatString("{0} {1}.", GET_COMMON_RC(Invalid_Null_Pointer), parameterName));
            Trace.WriteWarning(Uri::TraceCategory, "NamingUri::TryParse: {0}: {1}", innerError, innerError.Message);
            return innerError;
        }

        auto error = ParameterValidator::IsValid(
            name,
            ParameterValidator::MinStringSize,
            CommonConfig::GetConfig().MaxNamingUriLength);
        if (!error.IsSuccess())
        {
            ErrorCode innerError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), parameterName, ParameterValidator::MinStringSize, CommonConfig::GetConfig().MaxNamingUriLength));
            Trace.WriteWarning(Uri::TraceCategory, "NamingUri::TryParse: {0}: {1}", innerError, innerError.Message);
            return innerError;
        }

        std::wstring tempName(name);
        if (!NamingUri::TryParse(tempName, /*out*/nameUri))
        {
            ErrorCode innerError(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Uri), tempName));
            Trace.WriteWarning(Uri::TraceCategory, "NamingUri::TryParse: {0}: {1}", innerError, innerError.Message);
            return innerError;
        }

        return ErrorCode::Success();
    }

    ErrorCode NamingUri::TryParse(
        std::wstring const & nameText,
        std::wstring const & traceId,
        __out NamingUri & nameUri)
    {
        auto error = ParameterValidator::IsValid(
            nameText.c_str(),
            ParameterValidator::MinStringSize,
            CommonConfig::GetConfig().MaxNamingUriLength);
        if (!error.IsSuccess())
        {
            ErrorCode innerError(ErrorCodeValue::InvalidNameUri, wformatString(
                "{0} {1} {2}, {3}.", 
                GET_COMMON_RC(Invalid_LPCWSTR_Length), 
                nameText, 
                ParameterValidator::MinStringSize, 
                CommonConfig::GetConfig().MaxNamingUriLength));
            Trace.WriteInfo(Uri::TraceCategory, traceId, "NamingUri::TryParse: {0}: {1}", innerError, innerError.Message);
            return innerError;
        }

        if (!TryParse(nameText, nameUri))
        {
            ErrorCode innerError(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Uri), nameText));
            Trace.WriteWarning(Uri::TraceCategory, traceId, "NamingUri::TryParse: {0}: {1}", innerError, innerError.Message);
            return innerError;
        }

        return ErrorCode::Success();
    }

    bool NamingUri::IsNamingUri(Uri const & question)
    {
        if (question.Scheme != NameUriScheme)
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                "NamingUri '{0}' has invalid scheme: supported scheme is 'fabric:'.",
                question);
            return false;
        }

        if ((question.Type != UriType::Absolute) && (question.Type != UriType::Empty))
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                "NamingUri '{0}' is invalid: 'fabric:' scheme requires absolute path.",
                question);
            return false;
        }

        // Disallow empty segments
        if (StringUtility::Contains<wstring>(question.Path, L"//"))
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                "NamingUri '{0}' is invalid: empty segments in path are not supported.",
                question);
            return false;
        }

        // List of reserved characters
        for (auto delimiterIt = ReservedTokenDelimiters->begin(); delimiterIt != ReservedTokenDelimiters->end(); ++delimiterIt)
        {
            wstring delimiter = wformatString("{0}", *delimiterIt);
            if (StringUtility::Contains<wstring>(question.Path, delimiter))
            {
                Trace.WriteWarning(
                    Uri::TraceCategory,
                    "NamingUri '{0}' is invalid: '{1}' is a reserved character.",
                    question,
                    delimiter);
                return false;
            }
        }

        // Disallow trailing slash
        if ((question.Path.size() > 0) && (question.Path[question.Path.size() - 1] == L'/'))
        {
            Trace.WriteWarning(
                Uri::TraceCategory,
                "NamingUri '{0}' is invalid: trailing '/' is not supported.",
                question);
            return false;
        }

        return true;
    }

    ErrorCode NamingUri::ValidateName(NamingUri const & uri, std::wstring const & uriString, bool allowFragment)
    {
        if (uri == NamingUri::RootNamingUri)
        {
            return ErrorCode(
                ErrorCodeValue::AccessDenied,
                wformatString(GET_COMMON_RC(InvalidOperationOnRootNameURI), uriString));
        }

        if (uriString.size() > static_cast<size_t>(CommonConfig::GetConfig().MaxNamingUriLength))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidNameUri,
                wformatString(GET_COMMON_RC(InvalidNameExceedsMaxSize), uriString, uriString.size(), CommonConfig::GetConfig().MaxNamingUriLength));
        }

        if (!uri.Query.empty())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidNameUri,
                wformatString(GET_COMMON_RC(InvalidNameQueryCharacter), uriString));
        }

        if (allowFragment && !uri.Fragment.empty())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidNameUri,
                wformatString(GET_COMMON_RC(InvalidNameServiceGroupCharacter), uriString));
        }

        return ErrorCode::Success();
    }

    bool NamingUri::TryCombine(std::wstring const & path, __out NamingUri & result) const
    {
        wstring temp(this->ToString());

        if (!path.empty())
        {
            temp.append(L"/");
            temp.append(path);
        }

        NamingUri uri;
        if (TryParse(temp, uri))
        {
            result = move(uri);
            return true;
        }

        return false;
    }

    NamingUri NamingUri::Combine(NamingUri const & name, wstring const & path)
    {
        NamingUri result;
        if (!name.TryCombine(path, result))
        {
            Assert::CodingError("could not combine {0} and {1} into a Naming Uri", name, path);
        }

        return result;
    }

    ErrorCode NamingUri::FabricNameToId(std::wstring const & name, __inout std::wstring &escapedId)
    {
        std::wstring temp = name;
        StringUtility::TrimLeading<std::wstring>(temp, NamingUri::RootNamingUriString);
        StringUtility::TrimLeading<std::wstring>(temp, NamingUri::SegmentDelimiter);

        return EscapeString(temp, escapedId);
    }

    ErrorCode NamingUri::FabricNameToId(std::wstring const & name, bool useDelimiter, __inout std::wstring &escapedId)
    {
        std::wstring temp = name;
        std::wstring tempId;
        NamingUri::FabricNameToId(temp, tempId);
        if (useDelimiter)
        {
            StringUtility::Replace<std::wstring>(tempId, NamingUri::SegmentDelimiter, NamingUri::HierarchicalNameDelimiter);
        }

        return EscapeString(tempId, escapedId);
    }

    ErrorCode NamingUri::IdToFabricName(std::wstring const& scheme, std::wstring const& id, __inout std::wstring &name)
    {
        ASSERT_IF(scheme.empty(), "IdToFabricName - scheme cannot be empty");

        std::wstring escapedId;
        auto error = UnescapeString(id, escapedId);
        if (!error.IsSuccess())
        {
            return error;
        }

        StringUtility::Replace<std::wstring>(escapedId, NamingUri::HierarchicalNameDelimiter, NamingUri::SegmentDelimiter);
        name = wformatString("{0}{1}{2}", scheme, *NamingUri::SegmentDelimiter, escapedId);
        return ErrorCode::Success();
    }

    void NamingUri::ParseHost(std::wstring & input, std::wstring & host, std::wstring & remain)
    {
        input = input.substr(1, std::string::npos);
        std::size_t nextFound = remain.find(L"/");
        if (nextFound == std::string::npos)
        {
            nextFound = remain.find(L"?");
            if (nextFound == std::string::npos)
            {
                nextFound = remain.find(L"#");
                if (nextFound == std::string::npos)
                {
                    nextFound = input.length();
                }
            }
        }

        host = L"/";
        host = host + input.substr(0, nextFound);
        remain = input.substr(nextFound, std::string::npos);
    }

    void NamingUri::ParsePath(std::wstring & input, std::wstring & path, std::wstring & remain)
    {
        std::size_t nextFound = remain.find(L"?");
        if (nextFound == std::string::npos)
        {
            nextFound = remain.find(L"#");
            if (nextFound == std::string::npos)
            {
                nextFound = input.length();
            }
        }

        path = input.substr(0, nextFound);
        remain = input.substr(nextFound, std::string::npos);
    }

    ErrorCode NamingUri::ParseUnsafeUri(std::wstring const & input, std::wstring & protocol, std::wstring & host, std::wstring & path, std::wstring & queryFragment)
    {
        std::size_t protocolFound = input.find(L":/");
        std::wstring remain;
        if (protocolFound != std::string::npos)
        {
            protocol = input.substr(0, protocolFound + 2);
            remain = input.substr(protocolFound + 2, std::string::npos);
        }
        else
        {
            remain = input;
        }

        // Has host
        if (remain.length() > 0 && *(remain.begin()) == '/')
        {
            ParseHost(remain, host, remain);
        }

        ParsePath(remain, path, remain);
        queryFragment = remain;

        return ErrorCodeValue::Success;
    }

    ErrorCode NamingUri::EscapeString(std::wstring const & input, __inout std::wstring &output)
    {
#if defined(PLATFORM_UNIX)
        std::wstring protocol;
        std::wstring host;
        std::wstring path;
        std::wstring queryFragment;
        ErrorCode result = ParseUnsafeUri(input, protocol, host, path, queryFragment);

        std::string pathString;
        StringUtility::Utf16ToUtf8(path, pathString);
        static const char lookup[] = "0123456789ABCDEF";
        std::ostringstream out;
        for (std::string::size_type i = 0; i < pathString.length(); i++) {
            const char& c = pathString.at(i);
            if (c == '[' ||
                c == ']' ||
                c == '&' ||
                c == '^' ||
                c == '`' ||
                c == '{' ||
                c == '}' ||
                c == '|' ||
                c == '"' ||
                c == '<' ||
                c == '>' ||
                c == '\\' ||
                c == ' ')
            {
                out << '%';
                out << lookup[(c & 0xF0) >> 4];
                out << lookup[(c & 0x0F)];
            }
            else
            {
                out << c;
            }
        }
        std::wstring pathEscape;
        StringUtility::Utf8ToUtf16(out.str(), pathEscape);
        output = protocol + host + pathEscape + queryFragment;
#else
        DWORD size = 1;
        std::vector<WCHAR> buffer;
        buffer.resize(size);

        HRESULT hr = UrlEscape(const_cast<WCHAR*>(input.data()), buffer.data(), &size, URL_ESCAPE_AS_UTF8);
        if (FAILED(hr) && hr == E_POINTER)
        {
            buffer.resize(size);
            hr = UrlEscape(const_cast<WCHAR*>(input.data()), buffer.data(), &size, URL_ESCAPE_AS_UTF8);
            if (FAILED(hr))
            {
                return ErrorCode::FromHResult(hr);
            }
            output = buffer.data();
        }
        else
        {
            return ErrorCode::FromHResult(hr);
        }
#endif

        return ErrorCodeValue::Success;
    }

    ErrorCode NamingUri::UnescapeString(std::wstring const& input, __inout std::wstring& output)
    {
#if defined(PLATFORM_UNIX)
        std::wstring protocol;
        std::wstring host;
        std::wstring path;
        std::wstring queryFragment;
        ErrorCode result = ParseUnsafeUri(input, protocol, host, path, queryFragment);

        std::ostringstream out;
        std::string pathString;
        StringUtility::Utf16ToUtf8(path, pathString);
        for (std::string::size_type i = 0; i < pathString.length(); i++) {
            if (pathString.at(i) == '%') {
                std::string temp(pathString.substr(i + 1, 2));
                std::istringstream in(temp);
                short c = 0;
                in >> std::hex >> c;
                out << static_cast<unsigned char>(c);
                i += 2;
            }
            else {
                out << pathString.at(i);
            }
        }
        std::wstring pathUnescape;
        StringUtility::Utf8ToUtf16(out.str(), pathUnescape);
        output = protocol + host + pathUnescape + queryFragment;
#else
        DWORD size = 1;
        std::vector<WCHAR> buffer;
        buffer.resize(size);

        HRESULT hr = UrlUnescape(const_cast<WCHAR*>(input.data()), buffer.data(), &size, URL_UNESCAPE_AS_UTF8);
        if (FAILED(hr) && hr == E_POINTER)
        {
            buffer.resize(size);
            hr = UrlUnescape(const_cast<WCHAR*>(input.data()), buffer.data(), &size, URL_UNESCAPE_AS_UTF8);
            if (FAILED(hr))
            {
                return ErrorCodeValue::InvalidNameUri;
            }
            output = buffer.data();
        }
        else
        {
            return ErrorCodeValue::InvalidNameUri;
        }
#endif

        return ErrorCode::Success();
    }

    ErrorCode NamingUri::UrlEscapeString(std::wstring const & input, std::wstring & output)
    {
        // Clear the output because we are appending to it.
        output.clear();

        wstring basicEncodedStr;

        // First call escape string
        auto error = NamingUri::EscapeString(input, basicEncodedStr);

        // If escape returns an error, then return that error
        if (!error.IsSuccess())
        {
            return error;
        }

        // Replace chars with their encodings by appending
        // either the original char or its replacement to the output string
        // comma(",")
        // colon(":")
        // dollar("$")
        // plus sign("+")
        // semi - colon(";")
        // equals("=")
        // 'At' symbol("@")
        //
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
        // space(" ")
        // percent ("%") is not encoded here because it cannot be encoded twice.
        for (wchar_t c : basicEncodedStr)
        {
            if (c == L',')          // ,
            {
                output += L"%2C";
            }
            else if (c == L':')     // :
            {
                output += L"%3A";
            }
            else if (c == L'$')     // $
            {
                output += L"%24";
            }
            else if (c == L'+')     // +
            {
                output += L"%2B";
            }
            else if (c == L';')     // ;
            {
                output += L"%3B";
            }
            else if (c == L'=')     // =
            {
                output += L"%3D";
            }
            else if (c == L'@')     // @
            {
                output += L"%40";
            } // The following may never be called since EscapeString may have already taken care of them (OS dependent)
            else if (c == L'&')     // &
            {
                output += L"%26";
            }
            else if (c == L'/')     // / (forward slash)
            {
                output += L"%2F";
            }
            else if (c == L'\\')     // back slash
            {
                output += L"%5C";
            }
            else if (c == L'?')     // ?
            {
                output += L"%3F";
            }
            else if (c == L'#')     // #
            {
                output += L"%23";
            }
            else if (c == L'<')     // <
            {
                output += L"%3C";
            }
            else if (c == L'>')     // >
            {
                output += L"%3E";
            }
            else if (c == L'[')     // [
            {
                output += L"%5B";
            }
            else if (c == L']')     // ]
            {
                output += L"%5D";
            }
            else if (c == L'{')     // {
            {
                output += L"%7B";
            }
            else if (c == L'}')     // }
            {
                output += L"%7D";
            }
            else if (c == L'|')     // |
            {
                output += L"%7C";
            }            
            else if (c == L'^')     // ^
            {
                output += L"%5E";
            }
            else if (c == L' ')     // space
            {
                output += L"%20";
            }
            else
            {
                output += c;
            }
        }

        return ErrorCode::Success();
    }
}
