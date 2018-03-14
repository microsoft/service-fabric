// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class IQueryHandler
    {
    public:
        IQueryHandler() { };
        virtual ~IQueryHandler() { };

        bool Verify(Common::ComPointer<IFabricQueryClient7> const & queryClient, std::wstring const & queryName);

        virtual HRESULT BeginVerify(
            Common::ComPointer<IFabricQueryClient7> const & queryClient,
            Common::TimeSpan timeout,
            IFabricAsyncOperationCallback *callback,
            IFabricAsyncOperationContext **context);

        virtual HRESULT EndVerify(
            Common::ComPointer<IFabricQueryClient7> const & queryClient,
            IFabricAsyncOperationContext *context,
            __out bool & canRetry);

    protected:
        virtual bool Verify(
            Common::ComPointer<IFabricQueryClient7> const & queryClient,
            std::wstring const & queryName,
            __out bool & canRetry);
    };

    #define DECLARE_ASYNC_QUERY_HANDLER(QueryName, HandlerName)   \
    class HandlerName \
    : public IQueryHandler \
    , public std::enable_shared_from_this<HandlerName> \
    { \
    DENY_COPY(HandlerName)  \
    public: \
    HandlerName() : IQueryHandler()   \
    { \
    } \
    struct RegisterHandler \
    {   \
    RegisterHandler() \
    { \
    std::shared_ptr<HandlerName> handler = std::make_shared<HandlerName>(); \
    FabricTestQueryExecutor::AddQueryHandler(QueryName, handler); \
    } \
    };  \
    virtual HRESULT BeginVerify( \
        Common::ComPointer<IFabricQueryClient7> const & queryClient, \
        Common::TimeSpan timeout, \
        IFabricAsyncOperationCallback *callback, \
        IFabricAsyncOperationContext **context); \
    virtual HRESULT EndVerify( \
        Common::ComPointer<IFabricQueryClient7> const & queryClient, \
        IFabricAsyncOperationContext *context, \
        __out bool & canRetry); \
    static RegisterHandler registerHandler; \
    }; \
    HandlerName::RegisterHandler HandlerName::registerHandler;

    #define DECLARE_SYNC_QUERY_HANDLER(QueryName, HandlerName)   \
    class HandlerName \
    : public IQueryHandler \
    , public std::enable_shared_from_this<HandlerName> \
    { \
    DENY_COPY(HandlerName)  \
    public: \
    HandlerName() : IQueryHandler()   \
    { \
    } \
    struct RegisterHandler \
    {   \
    RegisterHandler() \
    { \
    std::shared_ptr<HandlerName> handler = std::make_shared<HandlerName>(); \
    FabricTestQueryExecutor::AddQueryHandler(QueryName, handler); \
    } \
    };  \
    protected: \
        virtual bool Verify( \
            Common::ComPointer<IFabricQueryClient7> const & queryClient, \
            std::wstring const & queryName, \
            __out bool & canRetry); \
    static RegisterHandler registerHandler; \
    }; \
    HandlerName::RegisterHandler HandlerName::registerHandler;
}
