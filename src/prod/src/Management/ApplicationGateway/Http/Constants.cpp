// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace HttpApplicationGateway;

GlobalWString Constants::HttpAppGatewayTraceId = make_global<wstring>(L"HttpApplicationGateway");


GlobalWString Constants::PartitionKind = make_global<wstring>(L"PartitionKind");

// TODO: Check if these strings are ok and consistent with the other places we ask for partitionkind
GlobalWString Constants::PartitionKindNamed = make_global<wstring>(L"Named");
GlobalWString Constants::PartitionKindInt64 = make_global<wstring>(L"Int64Range");
GlobalWString Constants::PartitionKindSingleton = make_global<wstring>(L"Singleton");
GlobalWString Constants::PartitionKey = make_global<wstring>(L"PartitionKey");
GlobalWString Constants::ServiceName = make_global<wstring>(L"ServiceName");

GlobalWString Constants::TimeoutString = make_global<wstring>(L"Timeout");
GlobalWString Constants::TargetReplicaSelectorString = make_global<wstring>(L"TargetReplicaSelector");
GlobalWString Constants::ListenerName = make_global<wstring>(L"ListenerName");

GlobalWString Constants::QueryStringDelimiter = make_global<wstring>(L"?");
GlobalWString Constants::FragmentDelimiter = make_global<wstring>(L"#");

//
// Custom service fabric headers
//
GlobalWString Constants::ServiceFabricHttpHeaderName = make_global<wstring>(L"X-ServiceFabric");
GlobalWString Constants::ServiceFabricHttpHeaderValueNotFound = make_global<wstring>(L"ResourceNotFound");

GlobalWString Constants::TracingConstantIndirection = make_global<wstring>(L"->");
GlobalWString Constants::TracingConstantOutdirection = make_global<wstring>(L"<-");
GlobalWString Constants::TracingConstantBothdirections = make_global<wstring>(L"<->");

GlobalKWString Constants::HttpConnectionHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Connection");
GlobalKWString Constants::HttpKeepAliveHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Keep-Alive");
GlobalKWString Constants::HttpProxyAuthenticateHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Proxy-Authenticate");
GlobalKWString Constants::HttpHostHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Host");
GlobalKWString Constants::HttpXForwardedHostHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"X-Forwarded-Host");
GlobalKWString Constants::HttpXForwardedForHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"X-Forwarded-For");
GlobalKWString Constants::HttpXForwardedProtoHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"X-Forwarded-Proto");
GlobalKWString Constants::HttpWebSocketSubprotocolHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Sec-WebSocket-Protocol");
GlobalKWString Constants::HttpAuthorizationHeaderName = make_global<KWString>(Utility::GetCurrentKtlAllocator(), L"Authorization");

GlobalWString Constants::HttpClientCertHeaderName = make_global<wstring>(L"X-Client-Certificate");

//
// Reverse proxy server certificate validation options
//
GlobalWString Constants::CertValidationNone = make_global<wstring>(L"None");
GlobalWString Constants::CertValidationServiceCertificateThumbprints = make_global<wstring>(L"ServiceCertificateThumbprints");
GlobalWString Constants::CertValidationServiceCommonNameAndIssuer = make_global<wstring>(L"ServiceCommonNameAndIssuer");
