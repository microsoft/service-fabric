// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name

// Traces will go to the DataMessaging channel only
#define HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, HttpApplicationGateway, trace_id, trace_level, Common::TraceChannelType::Debug, Common::TraceKeywords::DataMessaging, __VA_ARGS__),

// Traces will go to Default and DataMessagingAll channels, not the DataMessaging (critical) channel
#define HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, HttpApplicationGateway, trace_id, trace_level, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, DataMessagingAll), __VA_ARGS__),

// Internal tracing: Traces will go to the Default channel (Service fabric traces, but not the DataMessaging/DataMessagingAll channels)
#define HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, HttpApplicationGateway, trace_id, trace_level, __VA_ARGS__),

namespace HttpApplicationGateway
{
    class HttpApplicationGatewayEventSource
        : IHttpApplicationGatewayEventSource
    {
        BEGIN_STRUCTURED_TRACES(HttpApplicationGatewayEventSource)

            // 0-3 are taken by the Write*() methods
            // Critical traces (usually indicating failure) related to request processing pipeline
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(ProcessRequestErrorPreSendWithDetails, 5, Warning, "{0} Error while processing request, cannot forward to service: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, error = {5}, message = {6}, phase = {7}, {8}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "error", "errorMessage", "processRequestPhase", "errorDetails")
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(ForwardRequestErrorWithDetails, 6, Warning, "{0} Error while forwarding request to service: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, forward url = {5}, response status code = {6}, description = {7}, phase = {8}, {9}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "resolvedServiceUrl", "statusCode", "description", "sendRequestPhase", "errorDetails")
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(ForwardRequestConnectionErrorWithDetails, 7, Warning, "{0} Connection error while forwarding request, disconnecting client: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, forward url = {5},  phase = {6}, internal error = {7}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "resolvedServiceUrl", "failurePhase", "winHttpError")
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(SendResponseErrorWithDetails, 8, Warning, "{0} Unable to send response to client: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, forward url = {5}, phase = {6}, error = {7}, internal error = {8}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "resolvedServiceUrl", "sendResponsePhase", "error", "winHttpError")
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(ReceiveResponseErrorWithDetails, 9, Warning, "{0} Failed to receive response from service, disconnecting client: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, forward url = {5}, phase = {6}, internal error {7}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "resolvedServiceUrl", "failurePhase", "winHttpError")
            HTTPAPPLICATIONGATEWAY_CRITICAL_TRACE(ProcessRequestErrorWithDetails, 10, Warning, "{0} Error while processing request: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, forward url = {5}, number of successful resolve attempts = {6}, error = {7}, message = {8}, phase = {9}, {10}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "resolvedServiceUrl", "resolveCount", "error", "errorMessage", "processRequestPhase", "errorDetails")

            // Traces to go into the verbose channels (DataMessagingAll and Default - Service fabric traces)
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(RequestReceived, 21, Info, "{0} Request url = {1}, verb = {2}, remote (client) address = {3}, resolved service url = {4}, request processing start time = {5}", "traceId", "requestUrl", "verb", "remoteAddress", "resolvedServiceUrl", "requestStartTime")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ForwardRequestErrorRetrying, 22, Info, "{0} Forwarding request to service failed, retrying with re-resolve: phase = {1}, error = {2}, internal error = {3}", "traceId", "sendRequestPhase", "error", "winHttpError")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(GetClientAddressError, 25, Info, "{0} unable to get the client address {1}", "traceId", "error")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(MissingHostHeader, 26, Info, "{0} 'HOST' header missing or read failed", "traceId")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(RequestReresolved, 27, Info, "{0} Re-resolved service url = {1}", "traceId", "requestUrl")

            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ProcessRequestErrorPreSend, 28, Warning, "{0} Error while processing request, cannot forward to service: request url = {1}, verb = {2}, remote (client) address = {3}, request processing start time = {4}, error = {5}, message = {6}, phase = {7}, {8}", "traceId", "requestUrl", "verb", "remoteAddress", "requestStartTime", "error", "errorMessage", "processRequestPhase", "errorDetails")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ForwardRequestError, 29, Warning, "{0} Error while forwarding request to service: response status code = {1}, description = {2}, phase = {3}, {4}", "traceId", "statusCode", "description", "sendRequestPhase", "errorDetails")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ForwardRequestConnectionError, 30, Warning, "{0} Connection error while forwarding request, disconnecting client: phase = {1}, internal error = {2}", "traceId", "failurePhase", "winHttpError")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(SendResponseError, 31, Warning, "{0} Unable to send response to client: phase = {1}, error = {2}, internal error = {3}", "traceId", "sendResponsePhase", "error", "winHttpError")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ReceiveResponseError, 32, Warning, "{0} Failed to receive response from service, disconnecting client: phase = {1}, internal error {2}", "traceId", "failurePhase", "winHttpError")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ProcessRequestError, 33, Warning, "{0} Error while processing request: number of successful resolve attempts = {1}, error = {2}, message = {3}, phase = {4}, {5}", "traceId", "resolveCount", "error", "errorMessage", "processRequestPhase", "errorDetails")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ServiceResponseReresolve, 34, Info, "{0} Request to service returned: status code = {1}, status description = {2}, Reresolving", "traceId", "statusCode", "statusDescription")

            // Internal only traces related to request processing, these will not be shared in the DataMessaging channels
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(ServiceResponse, 42, Noise, "{0} Request to service returned: status code = {1}, status description = {2}", "traceId", "statusCode", "statusDescription")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(FoundChunkedTransferEncodingHeaderInRequest, 44, Noise, "{0} Found chunked transfer encoding header in client request", "traceId")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(FoundChunkedTransferEncodingHeaderInResponse, 45, Noise, "{0} Found chunked transfer encoding header in service response", "traceId")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(EndProcessReverseProxyRequestFailed, 46, Warning, "{0} HandlerAsyncOperation EndProcessReverseProxyRequest failed with {1}", "traceId", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(CreateHeaderValStrError, 47, Info, "{0} ProcessRequestAsyncOperation::PrepareRequest failed to create header value string, header {1}, error {2}", "traceId", "headerString", "error")

            // SSL connection traces
            // Certificate validation error traces logged when reverse proxy validates the certificate presented by the service
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ServiceCertValidationPolicyUnrecognizedError, 61, Warning, "{0} Failed to validate server certificate due to unrecognized policy: ApplicationCertificateValidationPolicy = {1}", "traceId", "configPolicyVal")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ServiceCertValidationThumbprintError, 62, Warning, "{0} Failed to validate server certificate due to invalid thumbprint: error = {1}", "traceId", "error")
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ServiceCertValidationError, 63, Warning, "{0} Failed to validate server certificate: error = {1}", "traceId", "status")
            // Get client certificate error, log at Info level since reverse proxy will proceed with the request 
            HTTPAPPLICATIONGATEWAY_VERBOSE_TRACE(ReadClientCertError, 64, Info, "{0} Read client request certificate failed: error = {1}, error message = {2}", "traceId", "error", "errorMessage")
            
            // Websocket related traces
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(WebsocketUpgradeError, 71, Noise, "{0} Websocket upgrade failed: error = {1}, winhttp error = {2}, safe to retry = {3}", "traceId", "error", "winHttpError", "safeToRetry")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(WebsocketUpgradeUnsupportedHeader, 72, Info, "{0} Websocket got request with unsupported upgrade header, upgrade header = {1}", "traceId", "upgradeHeader")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(WebsocketUpgradeCompleted, 73, Noise, "{0} Websocket upgrade complete, starting I/O", "traceId")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(WebsocketIOError, 74, Noise, "{0} Websocket I/O failed: error = {1}", "traceId", "error")

            // ServiceResolver traces, internal only, do not expose these traces in the DataMessaging/DataMessagingAll channels
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(CreateClientFactoryFail, 91, Warning, "Creation of fabricclient factory failed: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(NotificationRegistrationFail, 92, Warning, "Notification registration failed: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(GetServiceNameFail, 93, Warning, "{0} The ServiceName from URI cannot be parsed as Fabric URI: URI = {1}, error = {2}", "traceId", "urlSuffix", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(GetServiceReplicaSetFail, 94, Warning, "RSP for service returned with 0 endpoints: service name = {0}", "serviceName")

            // HttpApplicationGatewayImpl traces, internal only, do not expose these traces in the DataMessaging/DataMessagingAll channels
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(ServiceResolverOpenFailed, 101, Warning, "ServiceResolver open failed: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(BeginOpen, 102, Info, "HttpApplicationGatewayImpl BeginOpen")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(EndOpen, 103, Info, "Open completed: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(BeginClose, 104, Info, "HttpApplicationGatewayImpl BeginClose")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(EndClose, 105, Info, "Close completed: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(ParseListenAddressFail, 106, Error, "Failed to parse port: HttpApplicationGatewayListenAddress = {0}, error = {1}", "listenAddress", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(ParseGatewayAuthCredentialTypeFail, 107, Error, "Failed to parse GatewayAuthCredentialType: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(SecurityProviderError, 108, Error, "GatewayAuthCredentialType is not X509, cannot read client certificate: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(ParseGatewayX509CertError, 109, Error, "Failed to parse GatewayX509CertificateFindType and GatewayX509CertificateFindValue: error = {0}", "error")
            HTTPAPPLICATIONGATEWAY_INTERNAL_TRACE(GetGatewayCertificateError, 110, Error, "Error getting the GatewayX509Certificate: error = {0}", "error")
        END_STRUCTURED_TRACES
    public:

        // Functions to call the trace event
        DECLARE_TRACE_FUNC09(ProcessRequestErrorPreSendWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC10(ForwardRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, USHORT, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC08(ForwardRequestConnectionErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC09(SendResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_TRACE_FUNC08(ReceiveResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, uint64)
        DECLARE_TRACE_FUNC11(ProcessRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring)

        DECLARE_TRACE_FUNC09(ProcessRequestErrorPreSend, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC05(ForwardRequestError, std::wstring, USHORT, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC03(ForwardRequestConnectionError, std::wstring, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC04(SendResponseError, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_TRACE_FUNC03(ReceiveResponseError, std::wstring, std::wstring, uint64)
        DECLARE_TRACE_FUNC06(ProcessRequestError, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring)

        DECLARE_TRACE_FUNC06(RequestReceived, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime)
        DECLARE_TRACE_FUNC04(ForwardRequestErrorRetrying, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_TRACE_FUNC02(GetClientAddressError, std::wstring, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(MissingHostHeader, std::wstring)
        DECLARE_TRACE_FUNC02(RequestReresolved, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC03(ServiceResponseReresolve, std::wstring, USHORT, std::wstring)

        DECLARE_TRACE_FUNC03(ServiceResponse, std::wstring, USHORT, std::wstring)
        DECLARE_TRACE_FUNC01(FoundChunkedTransferEncodingHeaderInRequest, std::wstring)
        DECLARE_TRACE_FUNC01(FoundChunkedTransferEncodingHeaderInResponse, std::wstring)
        DECLARE_TRACE_FUNC02(EndProcessReverseProxyRequestFailed, std::wstring, Common::ErrorCode)
        DECLARE_TRACE_FUNC03(CreateHeaderValStrError, std::wstring, std::wstring, int)

        DECLARE_TRACE_FUNC02(ServiceCertValidationPolicyUnrecognizedError, std::wstring, int)
        DECLARE_TRACE_FUNC02(ServiceCertValidationThumbprintError, std::wstring, Common::ErrorCode)
        DECLARE_TRACE_FUNC02(ServiceCertValidationError, std::wstring, int)
        DECLARE_TRACE_FUNC03(ReadClientCertError, std::wstring, Common::ErrorCode, std::wstring)

        DECLARE_TRACE_FUNC04(WebsocketUpgradeError, std::wstring, Common::ErrorCode, uint64, bool)
        DECLARE_TRACE_FUNC02(WebsocketUpgradeUnsupportedHeader, std::wstring, std::wstring)
        DECLARE_TRACE_FUNC01(WebsocketUpgradeCompleted, std::wstring)
        DECLARE_TRACE_FUNC02(WebsocketIOError, std::wstring, Common::ErrorCode)

        DECLARE_TRACE_FUNC01(CreateClientFactoryFail, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(NotificationRegistrationFail, Common::ErrorCode)
        DECLARE_TRACE_FUNC03(GetServiceNameFail, std::wstring, std::wstring, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(GetServiceReplicaSetFail, std::wstring)

        DECLARE_TRACE_FUNC01(ServiceResolverOpenFailed, Common::ErrorCode)
        DECLARE_TRACE_FUNC00(BeginOpen)
        DECLARE_TRACE_FUNC01(EndOpen, Common::ErrorCode)
        DECLARE_TRACE_FUNC00(BeginClose)
        DECLARE_TRACE_FUNC01(EndClose, Common::ErrorCode)
        DECLARE_TRACE_FUNC02(ParseListenAddressFail, std::wstring, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(ParseGatewayAuthCredentialTypeFail, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(SecurityProviderError, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(ParseGatewayX509CertError, Common::ErrorCode)
        DECLARE_TRACE_FUNC01(GetGatewayCertificateError, Common::ErrorCode)

        // Declare structured trace
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ProcessRequestErrorPreSendWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ForwardRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, USHORT, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ForwardRequestConnectionErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(SendResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, Common::ErrorCode, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ReceiveResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, uint64);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ProcessRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ProcessRequestErrorPreSend, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ForwardRequestError, std::wstring, USHORT, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ForwardRequestConnectionError, std::wstring, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(SendResponseError, std::wstring, std::wstring, Common::ErrorCode, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ReceiveResponseError, std::wstring, std::wstring, uint64);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ProcessRequestError, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(RequestReceived, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ForwardRequestErrorRetrying, std::wstring, std::wstring, Common::ErrorCode, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(GetClientAddressError, std::wstring, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(MissingHostHeader, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(RequestReresolved, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceResponseReresolve, std::wstring, USHORT, std::wstring);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceResponse, std::wstring, USHORT, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(FoundChunkedTransferEncodingHeaderInRequest, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(FoundChunkedTransferEncodingHeaderInResponse, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(EndProcessReverseProxyRequestFailed, std::wstring, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(CreateHeaderValStrError, std::wstring, std::wstring, int);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceCertValidationPolicyUnrecognizedError, std::wstring, int);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceCertValidationThumbprintError, std::wstring, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceCertValidationError, std::wstring, int);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ReadClientCertError, std::wstring, Common::ErrorCode, std::wstring);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(WebsocketUpgradeError, std::wstring, Common::ErrorCode, uint64, bool);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(WebsocketUpgradeUnsupportedHeader, std::wstring, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(WebsocketUpgradeCompleted, std::wstring);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(WebsocketIOError, std::wstring, Common::ErrorCode);

        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(CreateClientFactoryFail, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(NotificationRegistrationFail, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(GetServiceNameFail, std::wstring, std::wstring, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(GetServiceReplicaSetFail, std::wstring);
        
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ServiceResolverOpenFailed, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(BeginOpen);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(EndOpen, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(BeginClose);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(EndClose, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ParseListenAddressFail, std::wstring, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ParseGatewayAuthCredentialTypeFail, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(SecurityProviderError, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(ParseGatewayX509CertError, Common::ErrorCode);
        DECLARE_HTTPAPPLICATIONGATEWAY_STRUCTURED_TRACE(GetGatewayCertificateError, Common::ErrorCode);

        static Common::Global<HttpApplicationGatewayEventSource> Trace;
    };
}
