// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Uri.h"

using namespace Common;
using namespace ServiceModel;
using namespace Query;
using namespace HttpGateway;

namespace HttpGateway
{
    class TestRequestHandler : public IHttpRequestHandler, public std::enable_shared_from_this<TestRequestHandler>
    {
    public:
        TestRequestHandler(wstring name) { name_ = name; }

        Common::AsyncOperationSPtr BeginProcessRequest(
            __in IRequestMessageContextUPtr request,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) 
        { 
            UNREFERENCED_PARAMETER(request);
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent); 
        }

        Common::ErrorCode EndProcessRequest(
            __in Common::AsyncOperationSPtr const& operation) { return CompletedAsyncOperation::End(operation); }

        wstring name_;
    };

    class HttpServerHandlerTests
    {
    public:
        static void CreatePrefixTree(HttpServerSPtr const & server)
        {
            server->prefixTreeUPtr_ = make_unique<HttpServer::PrefixTree>(make_unique<HttpServer::PrefixTreeNode>(HttpServer::NodeData(L"root")));
        }

        static ErrorCode SearchTree(
            HttpServerSPtr const & server,
            StringCollection const& tokenizedSuffix,
            ULONG &currentIndex,
            __out IHttpRequestHandlerSPtr &handler)
        {
            return server->SearchTree(server->prefixTreeUPtr_->Root, tokenizedSuffix, currentIndex, handler);
        }
    };

    BOOST_FIXTURE_TEST_SUITE(HttpServerHandlerTestsSuite,HttpServerHandlerTests)

    BOOST_AUTO_TEST_CASE(InsertionTest)
    {
        FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();
        HttpServerSPtr server;
        IHttpRequestHandlerSPtr handler;

        auto err = HttpServer::Create(config, server);
        VERIFY_IS_TRUE(err.IsSuccess());

        CreatePrefixTree(server);

        wstring path1 = L"/comp1/comp2/comp3/comp4/";
        shared_ptr<TestRequestHandler> handler1 = make_shared<TestRequestHandler>(path1);
        auto error = server->RegisterHandler(handler1->name_, handler1);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Try to register same path again
        //
        error = server->RegisterHandler(handler1->name_, handler1);
        VERIFY_IS_TRUE(!error.IsSuccess());

    }

    BOOST_AUTO_TEST_CASE(SearchTest)
    {
        FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();
        HttpServerSPtr server;
        ULONG index = 0;
        IHttpRequestHandlerSPtr handler;

        auto err = HttpServer::Create(config, server);
        VERIFY_IS_TRUE(err.IsSuccess());

        CreatePrefixTree(server);

        wstring path1 = L"/comp1/*/comp3/comp4/comp5/";
        wstring path2 = L"/comp1/comp2/comp3/comp4/";       
        
        wstring path3 = L"/comp1/comp2/comp3/comp4/comp5/";
        StringCollection tokenizedPath3;

        StringUtility::Split<wstring>(path3, tokenizedPath3, L"/");

        shared_ptr<TestRequestHandler> handler1 = make_shared<TestRequestHandler>(path1);
        shared_ptr<TestRequestHandler> handler2 = make_shared<TestRequestHandler>(path2);
        shared_ptr<TestRequestHandler> handler3;

        //
        // Register a generic handler - /comp1/*/comp3/comp4/comp5/
        //
        auto error = server->RegisterHandler(handler1->name_, handler1);
        VERIFY_IS_TRUE(error.IsSuccess());

        error = SearchTree(server, tokenizedPath3, index, handler);
        handler3 = dynamic_pointer_cast<TestRequestHandler>(handler);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Longest prefix match for /comp1/comp2/comp3/comp4/comp5/ should get the handler
        // that we registered above..
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(handler3->name_, path1));

        //
        // Register a more specific handler - /comp1/comp2/comp3/comp4/
        //
        error = server->RegisterHandler(handler2->name_, handler2);
        VERIFY_IS_TRUE(error.IsSuccess());
        index = 0;
        error = SearchTree(server, tokenizedPath3, index, handler);
        VERIFY_IS_TRUE(error.IsSuccess());
        handler3 = dynamic_pointer_cast<TestRequestHandler>(handler);

        //
        // Longest prefix match for /comp1/comp2/comp3/comp4/comp5/ should get the second handler.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(handler3->name_, path2));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
