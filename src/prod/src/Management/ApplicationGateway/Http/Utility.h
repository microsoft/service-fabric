// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class Utility
    {
    public:
        static Common::ErrorCode SuffixToFabricUri(
            std::wstring const& suffix, __out Common::NamingUri &name)
        {
            std::wstring unescapedSuffix;
            auto error = Common::NamingUri::UnescapeString(suffix, unescapedSuffix);
            if (!error.IsSuccess())
            {
                return error;
            }

            // Find first occurance of any of the Naming Uri reserved characters and remove all segments including and after that position.
            auto pos = unescapedSuffix.find_first_of(*Common::NamingUri::ReservedTokenDelimiters);
            if (pos != std::wstring::npos)
            {
                pos = unescapedSuffix.find_last_of(L"/", pos);
                if (pos != std::wstring::npos)
                {
                    unescapedSuffix.resize(pos);
                }
                else
                {
                    unescapedSuffix.clear();
                }
            }

            // NamingUri disallows empty segments so remove the empty segment and all segments following it.
            pos = unescapedSuffix.find(L"//");
            if (pos != std::wstring::npos)
            {
                unescapedSuffix.resize(pos);
            }

            Common::StringUtility::TrimTrailing<std::wstring>(unescapedSuffix, L"/");

            auto uriString = *Common::NamingUri::RootNamingUriString + L"/" + unescapedSuffix;
            HRESULT hr = Common::NamingUri::TryParse(
                uriString.c_str(),
                Constants::HttpAppGatewayTraceId, 
                name);
            if (FAILED(hr))
            {
                return Common::ErrorCode::FromHResult(hr);
            }

            return Common::ErrorCodeValue::Success;
        }

        static KAllocator& GetCurrentKtlAllocator()
        {
            return Common::GetSFDefaultPagedKAllocator();
        }

        //
        // This function changes the 'http' scheme in the uri to 'ws'. The caller has to
        // validate that the scheme of the input uri is 'http'
        //
        static std::wstring GetWebSocketUri(std::wstring const &uri)
        {
            std::wstring webSocketForwardingUri = uri;
            webSocketForwardingUri.replace(0, (*HttpCommon::HttpConstants::HttpUriScheme).size(), HttpCommon::HttpConstants::WebSocketUriScheme);

            return webSocketForwardingUri;
        }

        static KWString GetHeaderNameKString(Common::GlobalWString &headerName, KAllocator &allocator)
        {
            KWString headerNameStr(allocator);
            headerNameStr = KStringView(headerName->c_str());

            return Ktl::Move(headerNameStr);
        }

        static KMemRef AdjustBufferForCTE(KBuffer::SPtr &readBuffer)
        {
            PVOID address = readBuffer->GetBuffer();
            ULONG size = readBuffer->QuerySize();

            ASSERT_IF(size < cteSizeOverhead, "Insufficient space for CTE");

            // 1. Leave space for CTE header at the beginning.
            // 2. Leave room for CTE footer.
            return KMemRef((size - cteSizeOverhead), (PVOID)((PBYTE)address + cteHeaderSize));
        }

        static KMemRef GetCTECompletionChunk(KBuffer::SPtr &readBuffer)
        {
            PBYTE terminationChunk = (PBYTE)readBuffer->GetBuffer(); // 0\r\n\r\n
            terminationChunk[0] = '0';
            terminationChunk[1] = '\r';
            terminationChunk[2] = '\n';
            terminationChunk[3] = '\r';
            terminationChunk[4] = '\n';

            return KMemRef(5, readBuffer->GetBuffer(), 5);
        }

        //
        // Each chunk starts with the number of octets of the data it embeds expressed as a hexadecimal
        // number in ASCII followed by optional parameters (chunk extension) and a terminating CRLF sequence,
        // followed by the chunk data. The chunk is terminated by CRLF.
        // 
        // NOTE: This routine assumes that there is space before and after the buffer passed in to
        // add CTE information.
        //
        static KMemRef AddCTEInfo(__in KMemRef &memRef)
        {
            PBYTE cteHeader = ((PBYTE)memRef._Address - cteHeaderSize);
            ULONG chunkSize = memRef._Param;
            PBYTE cteFooter = ((PBYTE)memRef._Address + chunkSize);
            
            //
            // CTE header
            //

            // Write the 8 character size one nibble (4 bits) at a time
            cteHeader[0] = AsciiFromHex((chunkSize >> 28) & 0xf); // bytes 28 - 31
            cteHeader[1] = AsciiFromHex((chunkSize >> 24) & 0xf); // bytes 24 - 27
            cteHeader[2] = AsciiFromHex((chunkSize >> 20) & 0xf); // bytes 20 - 23
            cteHeader[3] = AsciiFromHex((chunkSize >> 16) & 0xf); // bytes 16 - 19
            cteHeader[4] = AsciiFromHex((chunkSize >> 12) & 0xf); // bytes 12 - 15
            cteHeader[5] = AsciiFromHex((chunkSize >> 8) & 0xf); // bytes 08 - 11
            cteHeader[6] = AsciiFromHex((chunkSize >> 4) & 0xf); // bytes 04 - 07
            cteHeader[7] = AsciiFromHex((chunkSize) & 0xf); // bytes 00 - 03

            // Add the new line after the CTE length
            cteHeader[8] = '\r';
            cteHeader[9] = '\n';

            //
            // CTE Footer
            //

            cteFooter[0] = '\r';
            cteFooter[1] = '\n';

            ULONG size = cteHeaderSize + chunkSize + cteFooterSize;
            return KMemRef(size, cteHeader, size);
        }

        //
        // This method combines the header values with a ','
        //
        static void CombineHeaderValues(KAllocator &allocator, KString::SPtr &existingHeaderValue, KString::SPtr &newHeaderValue, __out KString::SPtr &combinedHeaderValue)
        {
            ULONG size = newHeaderValue->Length() + 1;
            if (existingHeaderValue)
            {
                size = size + existingHeaderValue->Length() + 2; // including size of ", "
            }

            KString::Create(combinedHeaderValue, allocator, size);

            if (existingHeaderValue)
            {
                combinedHeaderValue->Concat(*existingHeaderValue);
                combinedHeaderValue->Concat(L", ");
            }

            combinedHeaderValue->Concat(*newHeaderValue);
        }

    private:

        static const ULONG cteBufferLengthSize = 8; // Number of characters reserved for sending the size of the CTE chunk in HEX(Max size - 0xFFFFFFFF).
        static const ULONG cteNewLineSize = 2; // \r\n
        static const ULONG cteHeaderSize = cteBufferLengthSize + cteNewLineSize;
        static const ULONG cteFooterSize = cteNewLineSize;
        static const ULONG cteSizeOverhead = cteHeaderSize + cteFooterSize;

        static inline char AsciiFromHex(BYTE const &value)
        {
            if (value < 10)
            {
                return static_cast<char>(value + '0');
            }
            else
            {
                return static_cast<char>(value - 10 + 'a');
            }
        }
    };
}	
