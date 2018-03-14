// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpCommon
{
    class HttpConstants
    {
    public:
        static ULONG AllocationTag;

        static ULONG DefaultEntityBodyChunkSize;
        static ULONG MaxEntityBodySize; // TODO: This should be allowd to be overridden by settings.xml
        static ULONG DefaultHeaderBufferSize;
        static ULONG StatusCodeGone;

        static Common::GlobalWString HttpUriScheme;
        static Common::GlobalWString HttpsUriScheme;
        static Common::GlobalWString WebSocketUriScheme;
        static Common::GlobalWString WebSocketSecureUriScheme;

        static Common::GlobalWString CurrentDefaultApiVersion;

        static Common::GlobalWString ContentTypeHeader;
        static Common::GlobalWString ContentLengthHeader;
        static Common::GlobalWString WWWAuthenticateHeader;
        static Common::GlobalWString TransferEncodingHeader;
        static Common::GlobalWString LocationHeader;
        static Common::GlobalWString AuthorizationHeader;
        static Common::GlobalWString HostHeader;
        static Common::GlobalWString UpgradeHeader;
        static Common::GlobalWString WebSocketSubprotocolHeader;
        static Common::GlobalWString ConnectionHeader;
        static Common::GlobalWString KeepAliveHeader;
        static Common::GlobalWString ProxyAuthenticateHeader;
		static Common::GlobalWString XAuthConfigHeader;
        static Common::GlobalWString XForwardedForHeader;
        static Common::GlobalWString XForwardedHostHeader;

        static Common::GlobalWString WebSocketUpgradeHeaderValue;

        static Common::GlobalWString ServiceFabricHttpClientRequestIdHeader;

        static Common::GlobalWString QueryStringDelimiter;
        static Common::GlobalWString SegmentDelimiter;
        static Common::GlobalWString ProtocolSeparator;

        static Common::GlobalWString ChunkedTransferEncoding;

        static Common::GlobalWString MatchAllSuffixSegment;

        //
        // Http verbs
        //
        static Common::GlobalWString HttpGetVerb;
        static Common::GlobalWString HttpPostVerb;
        static Common::GlobalWString HttpPutVerb;
        static Common::GlobalWString HttpDeleteVerb;
        static Common::GlobalWString HttpHeadVerb;
        static Common::GlobalWString HttpOptionsVerb;
        static Common::GlobalWString HttpTraceVerb;
        static Common::GlobalWString HttpConnectVerb;

        // Tag for ktl async context allocation
        static ULONG KtlAsyncDataContextTag;
    };
}
