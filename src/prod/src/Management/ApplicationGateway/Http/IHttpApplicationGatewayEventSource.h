// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_TRACE_FUNC00(trace_name) \
    void trace_name##Func() const\
    { trace_name(); } \

#define DECLARE_TRACE_FUNC01(trace_name, arg_type1) \
    void trace_name##Func(arg_type1 arg1) const\
    { trace_name(arg1); } \

#define DECLARE_TRACE_FUNC02(trace_name, arg_type1, arg_type2) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2) const\
    { trace_name(arg1, arg2); } \

#define DECLARE_TRACE_FUNC03(trace_name, arg_type1, arg_type2, arg_type3) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3) const\
    { trace_name(arg1, arg2, arg3); } \

#define DECLARE_TRACE_FUNC04(trace_name, arg_type1, arg_type2, arg_type3, arg_type4) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4) const\
    { trace_name(arg1, arg2, arg3, arg4); } \

#define DECLARE_TRACE_FUNC05(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5); } \

#define DECLARE_TRACE_FUNC06(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6); } \

#define DECLARE_TRACE_FUNC07(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6, arg_type7) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6, arg_type7 arg7) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6, arg7); } \

#define DECLARE_TRACE_FUNC08(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6, arg_type7, arg_type8) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6, arg_type7 arg7, arg_type8 arg8) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); } \

#define DECLARE_TRACE_FUNC09(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6, arg_type7, arg_type8, arg_type9) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6, arg_type7 arg7, arg_type8 arg8, arg_type9 arg9) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } \

#define DECLARE_TRACE_FUNC10(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6, arg_type7, arg_type8, arg_type9, arg_type10) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6, arg_type7 arg7, arg_type8 arg8, arg_type9 arg9, arg_type10 arg10) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); } \

#define DECLARE_TRACE_FUNC11(trace_name, arg_type1, arg_type2, arg_type3, arg_type4, arg_type5, arg_type6, arg_type7, arg_type8, arg_type9, arg_type10, arg_type11) \
    void trace_name##Func(arg_type1 arg1, arg_type2 arg2, arg_type3 arg3, arg_type4 arg4, arg_type5 arg5, arg_type6 arg6, arg_type7 arg7, arg_type8 arg8, arg_type9 arg9, arg_type10 arg10, arg_type11 arg11) const\
    { trace_name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11); } \

#define DECLARE_VIRTUAL_TRACE_FUNC(trace_name, ...) virtual void trace_name##Func(__VA_ARGS__) const =0;

namespace HttpApplicationGateway
{
    class IHttpApplicationGatewayEventSource
    {
    public:
        DECLARE_VIRTUAL_TRACE_FUNC(ProcessRequestErrorPreSendWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ForwardRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, USHORT, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ForwardRequestConnectionErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(SendResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ReceiveResponseErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, std::wstring, uint64)
        DECLARE_VIRTUAL_TRACE_FUNC(ProcessRequestErrorWithDetails, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring)

        DECLARE_VIRTUAL_TRACE_FUNC(ProcessRequestErrorPreSend, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::ErrorCode, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ForwardRequestError, std::wstring, USHORT, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ForwardRequestConnectionError, std::wstring, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(SendResponseError, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ReceiveResponseError, std::wstring, std::wstring, uint64)
        DECLARE_VIRTUAL_TRACE_FUNC(ProcessRequestError, std::wstring, uint, Common::ErrorCode, std::wstring, std::wstring, std::wstring)

        DECLARE_VIRTUAL_TRACE_FUNC(RequestReceived, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, Common::DateTime)
        DECLARE_VIRTUAL_TRACE_FUNC(ForwardRequestErrorRetrying, std::wstring, std::wstring, Common::ErrorCode, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(GetClientAddressError, std::wstring, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(MissingHostHeader, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(RequestReresolved, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(ServiceResponseReresolve, std::wstring, USHORT, std::wstring)

        DECLARE_VIRTUAL_TRACE_FUNC(ServiceResponse, std::wstring, USHORT, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(FoundChunkedTransferEncodingHeaderInRequest, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(FoundChunkedTransferEncodingHeaderInResponse, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(EndProcessReverseProxyRequestFailed, std::wstring, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(CreateHeaderValStrError, std::wstring, std::wstring, int)

        DECLARE_VIRTUAL_TRACE_FUNC(ServiceCertValidationPolicyUnrecognizedError, std::wstring, int)
        DECLARE_VIRTUAL_TRACE_FUNC(ServiceCertValidationThumbprintError, std::wstring, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(ServiceCertValidationError, std::wstring, int)
        DECLARE_VIRTUAL_TRACE_FUNC(ReadClientCertError, std::wstring, Common::ErrorCode, std::wstring)

        DECLARE_VIRTUAL_TRACE_FUNC(WebsocketUpgradeError, std::wstring, Common::ErrorCode, uint64, bool)
        DECLARE_VIRTUAL_TRACE_FUNC(WebsocketUpgradeUnsupportedHeader, std::wstring, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(WebsocketUpgradeCompleted, std::wstring)
        DECLARE_VIRTUAL_TRACE_FUNC(WebsocketIOError, std::wstring, Common::ErrorCode)

        DECLARE_VIRTUAL_TRACE_FUNC(CreateClientFactoryFail, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(NotificationRegistrationFail, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(GetServiceNameFail, std::wstring, std::wstring, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(GetServiceReplicaSetFail, std::wstring)

        DECLARE_VIRTUAL_TRACE_FUNC(ServiceResolverOpenFailed, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(BeginOpen)
        DECLARE_VIRTUAL_TRACE_FUNC(EndOpen, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(BeginClose)
        DECLARE_VIRTUAL_TRACE_FUNC(EndClose, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(ParseListenAddressFail, std::wstring, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(ParseGatewayAuthCredentialTypeFail, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(SecurityProviderError, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(ParseGatewayX509CertError, Common::ErrorCode)
        DECLARE_VIRTUAL_TRACE_FUNC(GetGatewayCertificateError, Common::ErrorCode)
    };
}
