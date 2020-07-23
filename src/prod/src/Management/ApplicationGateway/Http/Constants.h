// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    typedef Common::Global<KWString> GlobalKWString;

    class Constants
    {
    public:
        static Common::GlobalWString HttpAppGatewayTraceId;
        static Common::GlobalWString PartitionKind;
        static Common::GlobalWString PartitionKindNamed;
        static Common::GlobalWString PartitionKindInt64;
        static Common::GlobalWString PartitionKindSingleton;
        static Common::GlobalWString ServiceName;

        static Common::GlobalWString PartitionKey;

        static Common::GlobalWString TimeoutString;
        static Common::GlobalWString TargetReplicaSelectorString;
        static Common::GlobalWString QueryStringDelimiter;
        static Common::GlobalWString FragmentDelimiter;
        static Common::GlobalWString ListenerName;

        static Common::GlobalWString ServiceFabricHttpHeaderName;
        static Common::GlobalWString ServiceFabricHttpHeaderValueNotFound;

        static Common::GlobalWString TracingConstantIndirection;
        static Common::GlobalWString TracingConstantOutdirection;
        static Common::GlobalWString TracingConstantBothdirections;

        static GlobalKWString HttpConnectionHeaderName;
        static GlobalKWString HttpKeepAliveHeaderName;
        static GlobalKWString HttpProxyAuthenticateHeaderName;
        static GlobalKWString HttpHostHeaderName;
        static GlobalKWString HttpXForwardedHostHeaderName;
        static GlobalKWString HttpXForwardedForHeaderName;
        static GlobalKWString HttpXForwardedProtoHeaderName;
        static GlobalKWString HttpWebSocketSubprotocolHeaderName;
        static GlobalKWString HttpAuthorizationHeaderName;

        static Common::GlobalWString HttpClientCertHeaderName;

        static Common::GlobalWString CertValidationNone;
        static Common::GlobalWString CertValidationServiceCommonNameAndIssuer;
        static Common::GlobalWString CertValidationServiceCertificateThumbprints;
    };
}
