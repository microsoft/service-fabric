// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace HttpGateway;

namespace HttpGateway
{
    class UriArgumentParserTest
    {
    public:
        UriArgumentParserTest()
            : rootUrl_(L"http://localhost:80/")
            , path_(L"Nodes?api-version=1.0")
        {
        }

    protected:
        GatewayUri GetNodesUriWithNoParameters() const
        {
            return GetNodesGatewayUriInternal(path_);
        }

        GatewayUri GetNodesGatewayUri(wstring nodeStatus) const
        {
            auto nodeStatusFilterPath = path_ + L"&NodeStatusFilter=" + nodeStatus;

            return GetNodesGatewayUriInternal(nodeStatusFilterPath);
        }

        void ValidateNodeStatusFilterArgumentParameter(wstring uriNodeStatusFilterArgument, FABRIC_QUERY_NODE_STATUS_FILTER expectedParsedValue) const
        {
            DWORD nodeStatus = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT; //initialize to default

            auto uri = GetNodesGatewayUri(uriNodeStatusFilterArgument);
            UriArgumentParser argParser(uri);

            auto error = argParser.TryGetNodeStatusFilter(nodeStatus);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(nodeStatus == (DWORD)expectedParsedValue);
        }

    private:
        static vector<HandlerUriTemplate> GetHandlerUriTemplates()
        {
            HandlerUriTemplate handlerUri;
            handlerUri.SuffixPath = L"Nodes";
            handlerUri.Verb = Constants::HttpGetVerb;

            vector<HandlerUriTemplate> validHandlerUriVector;
            validHandlerUriVector.push_back(handlerUri);

            return validHandlerUriVector;
        }

        GatewayUri GetNodesGatewayUriInternal(wstring path) const
        {
            GatewayUri uri;
            VERIFY_IS_TRUE(GatewayUri::TryParse(GetHandlerUriTemplates(), Constants::HttpGetVerb, rootUrl_ + path, path, uri));

            return uri;
        }

    private:
        wstring const rootUrl_;
        wstring const path_;
    };

    BOOST_FIXTURE_TEST_SUITE(NodeStatusFilterParserTestSuite, UriArgumentParserTest)

    BOOST_AUTO_TEST_CASE(NoNodeStatusArgumentTest)
    {
        auto uri = GetNodesUriWithNoParameters();
        UriArgumentParser argParser(uri);

        DWORD nodeStatus = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;
        auto error = argParser.TryGetNodeStatusFilter(nodeStatus);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NameNotFound));
    }

    BOOST_AUTO_TEST_CASE(NodeStatusDefaultArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"default", FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT);
        ValidateNodeStatusFilterArgumentParameter(L"Default", FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT); // nodeStatusFilter parameter should be case insensitive
        ValidateNodeStatusFilterArgumentParameter(L"DEFAULT", FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusAllArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"all", FABRIC_QUERY_NODE_STATUS_FILTER_ALL);
        ValidateNodeStatusFilterArgumentParameter(L"All", FABRIC_QUERY_NODE_STATUS_FILTER_ALL);
        ValidateNodeStatusFilterArgumentParameter(L"ALL", FABRIC_QUERY_NODE_STATUS_FILTER_ALL);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusUpArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"up", FABRIC_QUERY_NODE_STATUS_FILTER_UP);
        ValidateNodeStatusFilterArgumentParameter(L"Up", FABRIC_QUERY_NODE_STATUS_FILTER_UP);
        ValidateNodeStatusFilterArgumentParameter(L"UP", FABRIC_QUERY_NODE_STATUS_FILTER_UP);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusDownArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"down", FABRIC_QUERY_NODE_STATUS_FILTER_DOWN);
        ValidateNodeStatusFilterArgumentParameter(L"Down", FABRIC_QUERY_NODE_STATUS_FILTER_DOWN);
        ValidateNodeStatusFilterArgumentParameter(L"DOWN", FABRIC_QUERY_NODE_STATUS_FILTER_DOWN);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusEnablingArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"enabling", FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING);
        ValidateNodeStatusFilterArgumentParameter(L"Enabling", FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING);
        ValidateNodeStatusFilterArgumentParameter(L"ENABLING", FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusDisablingArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"disabling", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING);
        ValidateNodeStatusFilterArgumentParameter(L"Disabling", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING);
        ValidateNodeStatusFilterArgumentParameter(L"DISABLING", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusDisabledArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"disabled", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED);
        ValidateNodeStatusFilterArgumentParameter(L"Disabled", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED);
        ValidateNodeStatusFilterArgumentParameter(L"DISABLED", FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusUnknownArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"unknown", FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN);
        ValidateNodeStatusFilterArgumentParameter(L"Unknown", FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN);
        ValidateNodeStatusFilterArgumentParameter(L"UNKNOWN", FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN);
    }

    BOOST_AUTO_TEST_CASE(NodeStatusRemovedArgumentParseTest)
    {
        ValidateNodeStatusFilterArgumentParameter(L"removed", FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED);
        ValidateNodeStatusFilterArgumentParameter(L"Removed", FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED);
        ValidateNodeStatusFilterArgumentParameter(L"REMOVED", FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED);
    }

    BOOST_AUTO_TEST_CASE(InvalidNodeStatusArgumentParseTest)
    {
        auto uri = GetNodesGatewayUri(L"InvalidNodeStatus");
        UriArgumentParser argParser(uri);

        DWORD nodeStatus = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;
        auto error = argParser.TryGetNodeStatusFilter(nodeStatus);

        VERIFY_IS_FALSE(error.IsSuccess());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));
    }
    BOOST_AUTO_TEST_SUITE_END()

}

