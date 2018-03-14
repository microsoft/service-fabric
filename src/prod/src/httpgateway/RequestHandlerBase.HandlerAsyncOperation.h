// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class RequestHandlerBase::HandlerAsyncOperation
        : public Common::AsyncOperation
#if !defined (PLATFORM_UNIX)
        , public HttpApplicationGateway::IHttpApplicationGatewayHandlerOperation
#endif
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(HandlerAsyncOperation);

    public:

        HandlerAsyncOperation(
            RequestHandlerBase & owner,
            HttpServer::IRequestMessageContextUPtr messageContext,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : Common::AsyncOperation(callback, parent)
            , messageContext_(std::move(messageContext))
            , owner_(owner)
            , timeout_(Common::TimeSpan::FromMinutes(Constants::DefaultFabricTimeoutMin))
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

        void OnError(Common::AsyncOperationSPtr const& thisSPtr, Common::ErrorCode const & error);
        void OnError(Common::AsyncOperationSPtr const& thisSPtr, __in USHORT statusCode, __in std::wstring const& statusDesc);
        void OnSuccess(Common::AsyncOperationSPtr const& thisSPtr, __in Common::ByteBufferUPtr body);
        void OnSuccess(Common::AsyncOperationSPtr const& thisSPtr, __in Common::ByteBufferUPtr body, __in std::wstring const& contentType);
        void OnSuccess(Common::AsyncOperationSPtr const& thisSPtr, __in Common::ByteBufferUPtr body, __in USHORT statusCode, __in std::wstring const& statusDesc);

        __declspec(property(get = get_MessageContext)) HttpServer::IRequestMessageContext &MessageContext;
        __declspec(property(get = get_MessageContextUPtr)) HttpServer::IRequestMessageContextUPtr &MessageContextUPtr;
        __declspec(property(get = get_MessageBody)) Common::ByteBufferUPtr const& Body;
        __declspec(property(get = get_Uri)) GatewayUri const& Uri;
        __declspec(property(get = get_Timeout)) Common::TimeSpan &Timeout;
        __declspec(property(get = get_FabricClient))  FabricClientWrapper& FabricClient;

        HttpServer::IRequestMessageContext & get_MessageContext() { return *messageContext_.get(); }
        HttpServer::IRequestMessageContextUPtr & get_MessageContextUPtr() { return messageContext_; }
        Common::ByteBufferUPtr const& get_MessageBody() const { return body_; }
        GatewayUri const& get_Uri() const { return uri_; }
        Common::TimeSpan const& get_Timeout() const { return timeout_; }
        FabricClientWrapper & get_FabricClient() { return *client_; }

        Common::ByteBufferUPtr && TakeBody() { return std::move(body_); }

        void SetFabricClient(__in FabricClientWrapperSPtr const&);

#if !defined (PLATFORM_UNIX)
        void SetAdditionalHeadersToSend(std::unordered_map<std::wstring, std::wstring>&&);
        void SetServiceName(std::wstring serviceName);

        // IHttpApplicationGatewayHandlerOperation methods
        void GetAdditionalHeadersToSend(std::unordered_map<std::wstring, std::wstring>&);
        void GetServiceName(std::wstring & serviceName);
#endif

        inline Common::JsonSerializerFlags GetSerializerFlags()
        {
            // Starting api version v3, we support serializing enum's as string and timespan in ISO format
            if (uri_.ApiVersion >= Constants::V3ApiVersion)
            {
                return static_cast<Common::JsonSerializerFlags>(
                        Common::JsonSerializerFlags::EnumInStringFormat |
                        Common::JsonSerializerFlags::DateTimeInIsoFormat);
            }
            else
            {
                return Common::JsonSerializerFlags::Default;
            }
        }

        template<typename T>
        inline Common::ErrorCode Serialize(T &object, __out_opt Common::ByteBufferUPtr &bytesUPtr)
        {
            return Common::JsonHelper::Serialize(object, bytesUPtr, GetSerializerFlags());
        }

        template<typename T>
        inline Common::ErrorCode Deserialize(T &object, __in Common::ByteBufferUPtr const &bytesUPtr)
        {
            return Common::JsonHelper::Deserialize(object, bytesUPtr, GetSerializerFlags());
        }

        template<typename T>
        inline Common::ErrorCode Deserialize(T &object, __in std::wstring const &str)
        {
            return Common::JsonHelper::Deserialize(object, str, GetSerializerFlags());
        }

    protected:
        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void LogRequest();
        virtual void GetRequestBody(Common::AsyncOperationSPtr const& thisSPtr);

    private:
        void OnAccessCheckComplete(Common::AsyncOperationSPtr const& thisSPtr, __in bool expectedCompletedSynchronously);
        void OnGetBodyComplete(Common::AsyncOperationSPtr const& thisSPtr, __in bool expectedCompletedSynchronously);
        void InvokeHandler(Common::AsyncOperationSPtr const& thisSPtr, __in Transport::RoleMask::Enum role);

        void UpdateRequestTimeout();

        // Set Content-Type and X-Content-Type-Options headers on the response
        Common::ErrorCode SetContentTypeResponseHeaders(__in std::wstring const& contentType);

        //
        // Helper routine to get the message body. This is used in during tracing.
        //
        std::string GetMessageBodyForTrace();
  
        GatewayUri uri_;
        HttpServer::IRequestMessageContextUPtr messageContext_;
        Common::ByteBufferUPtr body_;
        Common::TimeSpan timeout_;
        RequestHandlerBase & owner_;
        FabricClientWrapperSPtr client_;

#if !defined (PLATFORM_UNIX)
        std::unordered_map<std::wstring, std::wstring> additionalHeaders_;
        std::wstring serviceName_;
#endif
    };
}
