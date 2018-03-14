// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpCommon;

ULONG HttpConstants::AllocationTag = 'ttHF'; // Fabric Http allocation
ULONG HttpConstants::DefaultEntityBodyChunkSize = 2048;
ULONG HttpConstants::MaxEntityBodySize = 4 * 1024 * 1024;
ULONG HttpConstants::DefaultHeaderBufferSize = 8192;
ULONG HttpConstants::StatusCodeGone = 410;

GlobalWString HttpConstants::HttpUriScheme = make_global<wstring>(L"http");
GlobalWString HttpConstants::HttpsUriScheme = make_global<wstring>(L"https");
GlobalWString HttpConstants::WebSocketUriScheme = make_global<wstring>(L"ws");
GlobalWString HttpConstants::WebSocketSecureUriScheme = make_global<wstring>(L"wss");

GlobalWString HttpConstants::CurrentDefaultApiVersion = make_global<wstring>(L"1.0");

GlobalWString HttpConstants::ContentTypeHeader = make_global<wstring>(L"Content-Type");
GlobalWString HttpConstants::ContentLengthHeader = make_global<wstring>(L"Content-Length");
GlobalWString HttpConstants::WWWAuthenticateHeader = make_global<wstring>(L"WWW-Authenticate");
GlobalWString HttpConstants::TransferEncodingHeader = make_global<wstring>(L"Transfer-Encoding");
GlobalWString HttpConstants::LocationHeader = make_global<wstring>(L"Location");
GlobalWString HttpConstants::AuthorizationHeader = make_global<wstring>(L"Authorization");
GlobalWString HttpConstants::HostHeader = make_global<wstring>(L"Host");
GlobalWString HttpConstants::UpgradeHeader = make_global<wstring>(L"Upgrade");
GlobalWString HttpConstants::WebSocketSubprotocolHeader = make_global<wstring>(L"Sec-WebSocket-Protocol");
GlobalWString HttpConstants::ConnectionHeader = make_global<wstring>(L"Connection");
GlobalWString HttpConstants::KeepAliveHeader = make_global<wstring>(L"Keep-Alive");
GlobalWString HttpConstants::ProxyAuthenticateHeader = make_global<wstring>(L"Proxy-Authenticate");
GlobalWString HttpConstants::XAuthConfigHeader = make_global<wstring>(L"X-Registry-Auth");
GlobalWString HttpConstants::XForwardedHostHeader = make_global<wstring>(L"X-Forwarded-Host");
GlobalWString HttpConstants::XForwardedForHeader = make_global<wstring>(L"X-Forwarded-For");

GlobalWString HttpConstants::WebSocketUpgradeHeaderValue = make_global<wstring>(L"websocket");
GlobalWString HttpConstants::ServiceFabricHttpClientRequestIdHeader = make_global<wstring>(L"X-ServiceFabricRequestId");
GlobalWString HttpConstants::ChunkedTransferEncoding = make_global<wstring>(L"chunked");

GlobalWString HttpConstants::SegmentDelimiter = make_global<wstring>(L"/");
GlobalWString HttpConstants::QueryStringDelimiter = make_global<wstring>(L"?");
GlobalWString HttpConstants::ProtocolSeparator = make_global<wstring>(L":");

GlobalWString HttpConstants::MatchAllSuffixSegment = make_global<wstring>(L"*");

GlobalWString HttpConstants::HttpGetVerb = make_global<wstring>(L"GET");
GlobalWString HttpConstants::HttpPostVerb = make_global<wstring>(L"POST");
GlobalWString HttpConstants::HttpPutVerb = make_global<wstring>(L"PUT");
GlobalWString HttpConstants::HttpDeleteVerb = make_global<wstring>(L"DELETE");
GlobalWString HttpConstants::HttpHeadVerb = make_global<wstring>(L"HEAD");
GlobalWString HttpConstants::HttpOptionsVerb = make_global<wstring>(L"OPTIONS");
GlobalWString HttpConstants::HttpTraceVerb = make_global<wstring>(L"TRACE");
GlobalWString HttpConstants::HttpConnectVerb = make_global<wstring>(L"CONNECT");


ULONG HttpConstants::KtlAsyncDataContextTag = 'pttH';

