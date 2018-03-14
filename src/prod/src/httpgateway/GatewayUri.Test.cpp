// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Uri.h"

using namespace HttpGateway;

namespace HttpGateway
{

   

    class RESTUriParseTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(RESTUriParseTestSuite,RESTUriParseTest)

    BOOST_AUTO_TEST_CASE(ParseInvalidRESTUrlTest)
    {
        GatewayUri temp;
        vector<HandlerUriTemplate> validHandlerUriVector;
        HandlerUriTemplate handlerUri;
        handlerUri.SuffixPath = L"one/{id1}/$/method";
        handlerUri.Verb = Constants::HttpGetVerb;
        validHandlerUriVector.push_back(handlerUri);
        handlerUri.SuffixPath = L"withVersion/{id2}";
        handlerUri.Verb = Constants::HttpPostVerb;
        handlerUri.MinApiVersion = 2.0;
        handlerUri.MaxApiVersion = 2.0;
        validHandlerUriVector.push_back(handlerUri);

        HttpGatewayConfig::GetConfig().VersionCheck = false;
        wstring rootUrl1 = L"http://localhost:80/";
        wstring rootUrl2 = L"http://localhost:80/dir1/dir2/";
        wstring fail1 = L"this/should/fail";
        wstring fail2 = L"one/1/$$/method";
        wstring fail3 = L"one/1/two/$/invalidmethod";
        wstring fail4 = L"one/{id1}/two"; // no method
        wstring pass1 = L"one/1/two/$/method";
        wstring pass2 = L"withVersion/two";

        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+fail1, fail1, temp));
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+fail2, fail2, temp));
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+fail3, fail3, temp));
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+fail4, fail4, temp));

        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+pass1, pass1, temp));
        wstring value;
        VERIFY_IS_TRUE(temp.GetItem(L"id1", value));
        VERIFY_IS_TRUE(value == wstring(L"1/two"));
        // Verb match test
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpPostVerb, rootUrl1+pass1, pass1, temp));
        VERIFY_IS_FALSE(temp.GetItem(L"id2", value));

        HttpGatewayConfig::GetConfig().VersionCheck = true;
        pass2 = pass2 + L"?api-version=2.0";
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+pass1, pass1, temp));
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpPostVerb, rootUrl1+pass2, pass2, temp));

    }

    BOOST_AUTO_TEST_CASE(ParseValidRESTUrlTest)
    {
        GatewayUri temp;
        vector<HandlerUriTemplate> validHandlerUriVector;
        HandlerUriTemplate handlerUri;

        wstring rootUrl1 = L"http://localhost:80/";
        wstring rootUrl2 = L"http://localhost:80/dir1/dir2/";
        
        wstring appId = L"Applications1";
        wstring applications = L"Applications/" + appId;
        wstring serviceId = L"Services1";
        wstring services = L"/$/GetServices/" + serviceId;
        wstring partitionId = L"12345";
        wstring partitions = L"/$/GetsPartitions/" + partitionId;
        wstring applicationsUpgrade = applications + L"/" + L"$/Upgrade";

        handlerUri.SuffixPath = L"Applications/{"+ appId + L"}" + L"/$/Upgrade";
        handlerUri.Verb = Constants::HttpGetVerb;
        validHandlerUriVector.push_back(handlerUri);

        handlerUri.SuffixPath = L"Applications/{"+ appId + L"}" + L"/$/GetServices/{serviceId}";
        handlerUri.Verb = Constants::HttpPostVerb;
        validHandlerUriVector.push_back(handlerUri);

        HttpGatewayConfig::GetConfig().VersionCheck = false;
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+applicationsUpgrade, applicationsUpgrade, temp));
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl2+applicationsUpgrade, applicationsUpgrade, temp));
        wstring value;
        VERIFY_IS_TRUE(temp.GetItem(L"Applications1", value));
        VERIFY_IS_TRUE(value == appId);

        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+applications+services, applications+services, temp));
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpPostVerb, rootUrl1+applications+services, applications+services, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"serviceId", value));
        VERIFY_IS_TRUE(value == serviceId);           

        // strings with ?Query#fragment.
        HttpGatewayConfig::GetConfig().VersionCheck = true;
        handlerUri.SuffixPath = L"";
        handlerUri.Verb = Constants::HttpGetVerb;
        handlerUri.MinApiVersion = 2.0;
        handlerUri.MaxApiVersion = 2.0;
        validHandlerUriVector.push_back(handlerUri);

        handlerUri.SuffixPath = L"a/{val}/$/c";
        handlerUri.Verb = Constants::HttpGetVerb;
        handlerUri.MinApiVersion = 2.0;
        handlerUri.MaxApiVersion = 2.0;
        validHandlerUriVector.push_back(handlerUri);

        wstring path1 = L"?api-version=2.0";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path1, path1, temp));
        wstring path2 = L"a/b/testvalue/$/c?api-version=2.0";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path2, path2, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"val", value));
        VERIFY_IS_TRUE(value == L"b/testvalue");

        wstring path0 = L"?api-version=1.0";
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path0, path0, temp));

        wstring path3 = L"a/b/testvalue/$/c?api-version=2.0&item1=one&item2=two";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path3, path3, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"item1", value));
        VERIFY_IS_TRUE(value == L"one");
        VERIFY_IS_TRUE(temp.GetItem(L"item2", value));
        VERIFY_IS_TRUE(value == L"two");

        // Verify that values in querystring cannot override the values set by values during earlier parsing.
        wstring path4 = L"a/b/testvalue/$/c?api-version=2.0&item1=one&item2&val=temp";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path4, path4, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"val", value));
        VERIFY_IS_TRUE(value == L"b/testvalue");

        // uri's with fragment
        wstring path5 = L"a/b/testvalue/$/c?api-version=2.0#tempfragment";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path5, path5, temp));

        wstring path6 = L"a/b/testvalue/$/c#tempfragment?api-version=2.0";
        VERIFY_IS_FALSE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path6, path6, temp));
        
        HttpGatewayConfig::GetConfig().VersionCheck = false;
        wstring path7 = L"a/b/testvalue/$/c#tempfragment";
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path7, path7, temp));

        const Common::StringLiteral TTraceType = "TraceType";
        handlerUri.SuffixPath = L"Faults/Nodes/{val}/$/h";
        handlerUri.Verb = Constants::HttpGetVerb;
        validHandlerUriVector.push_back(handlerUri);

        wstring path8 = L"Faults/Nodes/Node999/$/h";

        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path8, path8, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"val", value));
        VERIFY_IS_TRUE(value == L"Node999");

        wstring path9 = L"Faults/";
        handlerUri.SuffixPath = L"Faults/";
        handlerUri.Verb = Constants::HttpGetVerb;
        validHandlerUriVector.push_back(handlerUri);
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpGetVerb, rootUrl1+path9, path9, temp));

        wstring path10 = L"Faults/Nodes/Node123/$/StartTransition?api-version=1.0&OperationId=5a7896d1-6308-4580-a632-5f7a7bbc046e&NodeTransitionType=Stop&NodeInstanceId=131263072020024832&StopDurationInSeconds=9999";
        handlerUri.SuffixPath = L"Faults/Nodes/{NodeName}/$/StartTransition";
        handlerUri.Verb = Constants::HttpPostVerb;
        validHandlerUriVector.push_back(handlerUri);
        VERIFY_IS_TRUE(GatewayUri::TryParse(validHandlerUriVector, Constants::HttpPostVerb, rootUrl1+path10, path10, temp));
        VERIFY_IS_TRUE(temp.GetItem(L"OperationId", value));
        VERIFY_IS_TRUE(value == L"5a7896d1-6308-4580-a632-5f7a7bbc046e");
        VERIFY_IS_TRUE(temp.GetItem(L"NodeName", value));
        VERIFY_IS_TRUE(value == L"Node123");
        VERIFY_IS_FALSE(value == L"Node321");
    }

    BOOST_AUTO_TEST_SUITE_END()

}

