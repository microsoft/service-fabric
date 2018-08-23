// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
#if !defined(PLATFORM_UNIX)
using namespace HttpClient;
using namespace HttpCommon;
#endif

const StringLiteral TraceType("IPAMTest");

#if !defined(PLATFORM_UNIX)
class MockClientRequest : public IHttpClientRequest
{
public:
    MockClientRequest(
        wstring uri,
        wstring verb,
        const string &msgBody,
        unsigned long expectedSendStatus,
        unsigned long expectedResponseStatus,
        int expectedSendError,
        int expectedResponseError)
        : uri_(uri),
        expectedVerb_(verb),
        msgBody_(msgBody),
        sendStatus_(expectedSendStatus),
        responseStatus_(expectedResponseStatus),
        sendError_(expectedSendError),
        responseError_(expectedResponseError) { };

    // Inherited via IHttpClientRequest
    virtual KAllocator & GetThisAllocator() const override
    {
        VERIFY_IS_TRUE(false);

        throw 0;
    }

    virtual wstring GetRequestUrl() const override
    {
        return uri_;
    }

    virtual BOOLEAN IsSecure() const override
    {
        return FALSE;
    }

    virtual void SetVerb(wstring const & verb) override
    {
        VERIFY_ARE_EQUAL(verb, expectedVerb_);
    }

    virtual void SetVerb(KHttpUtils::OperationType verb) override
    {
        UNREFERENCED_PARAMETER(verb);

        VERIFY_IS_TRUE(false);
    }

    virtual ErrorCode SetRequestHeaders(wstring const & headers) override
    {
        UNREFERENCED_PARAMETER(headers);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode SetRequestHeaders(KString::SPtr const & HeaderBlock) override
    {
        UNREFERENCED_PARAMETER(HeaderBlock);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode SetRequestHeader(
        wstring const & headerName,
        wstring const & headerValue) override
    {
        UNREFERENCED_PARAMETER(headerName);
        UNREFERENCED_PARAMETER(headerValue);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode SetRequestHeader(
        KStringView & headerName,
        KStringView & headerValue) override
    {
        UNREFERENCED_PARAMETER(headerName);
        UNREFERENCED_PARAMETER(headerValue);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode SetClientCertificate(PCCERT_CONTEXT pCertContext) override
    {
        UNREFERENCED_PARAMETER(pCertContext);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode SetServerCertValidationHandler(
        KHttpCliRequest::ServerCertValidationHandler handler) override
    {
        UNREFERENCED_PARAMETER(handler);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual AsyncOperationSPtr BeginSendRequest(
        ByteBufferUPtr body,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        UNREFERENCED_PARAMETER(body);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode((ErrorCodeValue::Enum)this->sendError_),
            callback,
            parent);
    }

    virtual AsyncOperationSPtr BeginSendRequest(
        KBuffer::SPtr && body,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        UNREFERENCED_PARAMETER(body);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::NotImplemented),
            callback,
            parent);
    }

    virtual ErrorCode EndSendRequest(
        AsyncOperationSPtr const & operation,
        ULONG * winHttpError) override
    {
        UNREFERENCED_PARAMETER(winHttpError);

        *winHttpError = this->sendStatus_;
        return AsyncOperation::End(operation);
    }

    virtual AsyncOperationSPtr BeginSendRequestHeaders(
        ULONG totalRequestLength,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        UNREFERENCED_PARAMETER(totalRequestLength);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        VERIFY_IS_TRUE(false);

        return nullptr;
    }

    virtual ErrorCode EndSendRequestHeaders(
        AsyncOperationSPtr const & operation,
        ULONG * winHttpError) override
    {
        UNREFERENCED_PARAMETER(operation);
        UNREFERENCED_PARAMETER(winHttpError);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual AsyncOperationSPtr BeginSendRequestChunk(
        KMemRef & memRef,
        bool isLastSegment,
        bool disconnect,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        UNREFERENCED_PARAMETER(memRef);
        UNREFERENCED_PARAMETER(isLastSegment);
        UNREFERENCED_PARAMETER(disconnect);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        VERIFY_IS_TRUE(false);

        return nullptr;
    }

    virtual ErrorCode EndSendRequestChunk(
        AsyncOperationSPtr const & operation,
        ULONG * winHttpError) override
    {
        UNREFERENCED_PARAMETER(operation);
        UNREFERENCED_PARAMETER(winHttpError);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode GetResponseStatusCode(
        USHORT * statusCode,
        wstring & description) const override
    {
        *statusCode = (unsigned short) this->responseStatus_;
        description = L"Success";
        return ErrorCode((ErrorCodeValue::Enum)this->responseError_);
    }

    virtual ErrorCode GetResponseHeader(
        wstring const & headerName,
        wstring & headerValue) const override
    {
        UNREFERENCED_PARAMETER(headerName);
        UNREFERENCED_PARAMETER(headerValue);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode GetResponseHeader(
        wstring const & headerName,
        KString::SPtr & headerValue) const override
    {
        UNREFERENCED_PARAMETER(headerName);
        UNREFERENCED_PARAMETER(headerValue);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode GetAllResponseHeaders(wstring & headers) override
    {
        UNREFERENCED_PARAMETER(headers);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode GetAllResponseHeaders(KString::SPtr & headerValue) override
    {
        UNREFERENCED_PARAMETER(headerValue);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual AsyncOperationSPtr BeginGetResponseBody(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode((ErrorCodeValue::Enum)this->responseError_),
            callback,
            parent);
    }

    virtual ErrorCode EndGetResponseBody(
        AsyncOperationSPtr const & operation,
        ByteBufferUPtr & body) override
    {
        body = make_unique<ByteBuffer>(msgBody_.size() + 1);
        copy(msgBody_.begin(), msgBody_.end(), body->begin());

        return AsyncOperation::End(operation);
    }

    virtual ErrorCode EndGetResponseBody(
        AsyncOperationSPtr const & operation,
        KBuffer::SPtr & body) override
    {
        UNREFERENCED_PARAMETER(operation);
        UNREFERENCED_PARAMETER(body);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual AsyncOperationSPtr BeginGetResponseChunk(
        KMemRef & mem,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) override
    {
        UNREFERENCED_PARAMETER(mem);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(ErrorCodeValue::NotImplemented),
            callback,
            parent);
    }

    virtual ErrorCode EndGetResponseChunk(
        AsyncOperationSPtr const & operation,
        KMemRef & mem,
        ULONG * winHttpError) override
    {
        UNREFERENCED_PARAMETER(operation);
        UNREFERENCED_PARAMETER(mem);
        UNREFERENCED_PARAMETER(winHttpError);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

private:
    wstring uri_;
    wstring expectedVerb_;
    const string msgBody_;
    unsigned long sendStatus_;
    unsigned long responseStatus_;
    int sendError_;
    int responseError_;
};

class MockHttpClient : public IHttpClient
{
public:
    MockHttpClient(
        wstring expectedVerb,
        vector<string> &refreshList,
        bool expectedSync,
        unsigned long expectedSendStatus,
        unsigned long expectedResponseStatus,
        int expectedSendError,
        int expectedResponseError)
        : verb_(expectedVerb),
        refreshBodyList_(refreshList),
        sync_(expectedSync),
        sendStatus_(expectedSendStatus),
        responseStatus_(expectedResponseStatus),
        sendError_(expectedSendError),
        responseError_(expectedResponseError),
        refreshIndex_(0) { };

    // Inherited via IHttpClient
    virtual ErrorCode CreateHttpRequest(
        wstring const & requestUri,
        TimeSpan const & connectTimeout,
        TimeSpan const & sendTimeout,
        TimeSpan const & receiveTimeout,
        KAllocator & allocator,
        IHttpClientRequestSPtr & clientRequest,
        bool allowRedirects = true,
        bool enableCookies = true,
        bool enableWinauthAutoLogon = false) override
    {
        UNREFERENCED_PARAMETER(requestUri);
        UNREFERENCED_PARAMETER(connectTimeout);
        UNREFERENCED_PARAMETER(sendTimeout);
        UNREFERENCED_PARAMETER(receiveTimeout);
        UNREFERENCED_PARAMETER(allocator);
        UNREFERENCED_PARAMETER(clientRequest);
        UNREFERENCED_PARAMETER(allowRedirects);
        UNREFERENCED_PARAMETER(enableCookies);
        UNREFERENCED_PARAMETER(enableWinauthAutoLogon);

        return ErrorCode(ErrorCodeValue::NotImplemented);
    }

    virtual ErrorCode CreateHttpRequest(
        wstring const & requestUri,
        TimeSpan const & connectTimeout,
        TimeSpan const & sendTimeout,
        TimeSpan const & receiveTimeout,
        IHttpClientRequestSPtr & clientRequest,
        bool allowRedirects = true,
        bool enableCookies = true,
        bool enableWinauthAutoLogon = false) override
    {
        UNREFERENCED_PARAMETER(connectTimeout);
        UNREFERENCED_PARAMETER(sendTimeout);
        UNREFERENCED_PARAMETER(receiveTimeout);
        UNREFERENCED_PARAMETER(allowRedirects);
        UNREFERENCED_PARAMETER(enableCookies);
        UNREFERENCED_PARAMETER(enableWinauthAutoLogon);

        const string &msgBody = this->refreshBodyList_[this->refreshIndex_];
        if (this->refreshIndex_ < this->refreshBodyList_.size() - 1)
        {
            // Stick on the last entry.
            //
            this->refreshIndex_++;
        }

        clientRequest = make_shared<MockClientRequest>(
            requestUri, 
            verb_, 
            msgBody,
            sendStatus_, 
            responseStatus_, 
            sendError_, 
            responseError_);

        return ErrorCode(ErrorCodeValue::Success);
    }

private:
    wstring verb_;
    bool sync_;
    unsigned long sendStatus_;
    unsigned long responseStatus_;
    int sendError_;
    int responseError_;
    const vector<string> refreshBodyList_;
    int refreshIndex_;
};

static const string testData =
    "<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
    "<IPSubnet Prefix=\"10.0.0.0 / 24\">"
    "<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
    "<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
    "</IPSubnet>"
    "</Interface></Interfaces>";

static const string testData_missing_10_0_0_6 =
    "<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
    "<IPSubnet Prefix=\"10.0.0.0 / 24\">"
    "<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
    "<IPAddress Address=\"10.0.0.5\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
    "</IPSubnet>"
    "</Interface></Interfaces>";

static const string badData =
    "<Interfaces><Interface MacAddress=\"000D3A73F074\" IsPrimary=\"true\">"
    "<IPSubnet Prefix=\"10.0.0.0 / 24\">"
    "<IPAddress Address=\"10.0.0.4\" IsPrimary=\"true\"/>"
    "<IPAddress Address=\"10.0.0.5\" IsPrimary=\"true\"/>"
    "<IPAddress Address=\"10.0.0.6\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.7\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.8\" IsPrimary=\"false\"/>"
    "<IPAddress Address=\"10.0.0.9\" IsPrimary=\"false\"/>"
    "</IPSubnet>"
    "</Interface></Interfaces>";

class IPAMTest
{
protected:
    IPAMTest() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup)
        ~IPAMTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup)

    void OnInitializeComplete(
        IPAM *ipam,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously);

    void VerifyCounts(IPAM *ipam, int total, int inuse, int avail);

    bool expectSync_;
    bool expectSuccess_;
};

BOOST_FIXTURE_TEST_SUITE(IPAMTestSuite, IPAMTest)

BOOST_AUTO_TEST_CASE(SynchronousInitializationSuccessTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        data, 
        true, 
        200, 
        200, 
        ErrorCodeValue::Success, 
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    expectSync_ = true;
    expectSuccess_ = true;

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);
}

BOOST_AUTO_TEST_CASE(SynchronousInitializationFailureTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);

    vector<string> bad;
    bad.push_back(badData);

    expectSync_ = true;
    expectSuccess_ = false;

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        data, 
        true, 
        200, 
        200, 
        ErrorCodeValue::CannotConnect, 
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        data, 
        true, 
        200, 
        200, 
        ErrorCodeValue::Success, 
        ErrorCodeValue::CannotConnect);
    ipam = new IPAMWindows(http, root);

    operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        data, 
        true, 
        200, 
        500, 
        ErrorCodeValue::Success, 
        ErrorCodeValue::Success);
    ipam = new IPAMWindows(http, root);

    operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        bad, 
        true, 
        200, 
        200, 
        ErrorCodeValue::Success, 
        ErrorCodeValue::Success);
    ipam = new IPAMWindows(http, root);

    operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);
}

BOOST_AUTO_TEST_CASE(SynchronousLoadAndReserveTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb, 
        data, 
        true, 
        200, 
        200, 
        ErrorCodeValue::Success, 
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    expectSync_ = true;
    expectSuccess_ = true;

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100), 
        [this, ipam](AsyncOperationSPtr const &operation)
        {
            this->OnInitializeComplete(ipam, operation, false);
        },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    // At this point everything is initialized.  Now validate that we can 
    // reserve the right number of IPs.
    //
    VerifyCounts(ipam, 5, 0, 5);

    uint ip1, ip2, ip3, ip4, ip5, ip6;
    auto error = ipam->Reserve(L"Reservation1", ip1);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyCounts(ipam, 5, 1, 4);

    error = ipam->Release(L"Reservation1");
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyCounts(ipam, 5, 0, 5);

    error = ipam->Reserve(L"Reservation1", ip2);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyCounts(ipam, 5, 1, 4);
    
    // Ensure that we do not see immediate reuse.
    VERIFY_ARE_NOT_EQUAL(ip1, ip2);

    error = ipam->Release(L"Reservation1");
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyCounts(ipam, 5, 0, 5);

    // Now exhaust the pool and see a reserve failure.
    //
    error = ipam->Reserve(L"Reservation1", ip1);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = ipam->Reserve(L"Reservation2", ip2);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = ipam->Reserve(L"Reservation3", ip3);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = ipam->Reserve(L"Reservation4", ip4);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = ipam->Reserve(L"Reservation5", ip5);
    VERIFY_IS_TRUE(error.IsSuccess());
    error = ipam->Reserve(L"Reservation6", ip6);
    VERIFY_IS_FALSE(error.IsSuccess());
    VERIFY_ARE_NOT_EQUAL(ip1, ip2);
    VERIFY_ARE_NOT_EQUAL(ip1, ip3);
    VERIFY_ARE_NOT_EQUAL(ip1, ip4);
    VERIFY_ARE_NOT_EQUAL(ip1, ip5);
    VERIFY_ARE_NOT_EQUAL(ip1, ip6);
    VERIFY_ARE_NOT_EQUAL(ip2, ip3);
    VERIFY_ARE_NOT_EQUAL(ip2, ip4);
    VERIFY_ARE_NOT_EQUAL(ip2, ip5);
    VERIFY_ARE_NOT_EQUAL(ip2, ip6);
    VERIFY_ARE_NOT_EQUAL(ip3, ip4);
    VERIFY_ARE_NOT_EQUAL(ip3, ip5);
    VERIFY_ARE_NOT_EQUAL(ip3, ip6);
    VERIFY_ARE_NOT_EQUAL(ip4, ip5);
    VERIFY_ARE_NOT_EQUAL(ip4, ip6);
    VERIFY_ARE_NOT_EQUAL(ip5, ip6);
}

BOOST_AUTO_TEST_CASE(SimpleRefreshTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb,
        data,
        true,
        200,
        200,
        ErrorCodeValue::Success,
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    expectSync_ = true;
    expectSuccess_ = true;

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100),
        [this, ipam](AsyncOperationSPtr const &operation)
    {
        this->OnInitializeComplete(ipam, operation, false);
    },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    // Now start the recurring refresh
    //
    auto lastLoad = ipam->LastRefreshTime;
    auto error = ipam->StartRefreshProcessing(TimeSpan::FromSeconds(2), TimeSpan::FromMilliseconds(100),
        [](DateTime lastRefresh)
        {
            UNREFERENCED_PARAMETER(lastRefresh);

            // There should never be a ghost
            VERIFY_IS_TRUE(false);
        });

    VERIFY_IS_TRUE(error.IsSuccess());
    ManualResetEvent waiter;
    waiter.Wait(TimeSpan::FromSeconds(5));
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    // Cancel refresh processing
    ipam->CancelRefreshProcessing();
}

BOOST_AUTO_TEST_CASE(SimpleRemovalTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);
    data.push_back(testData_missing_10_0_0_6);

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb,
        data,
        true,
        200,
        200,
        ErrorCodeValue::Success,
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    expectSync_ = true;
    expectSuccess_ = true;

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100),
        [this, ipam](AsyncOperationSPtr const &operation)
    {
        this->OnInitializeComplete(ipam, operation, false);
    },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    VerifyCounts(ipam, 5, 0, 5);

    // Now start the recurring refresh
    //
    auto lastLoad = ipam->LastRefreshTime;
    auto error = ipam->StartRefreshProcessing(TimeSpan::FromSeconds(2), TimeSpan::FromMilliseconds(100),
        [](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);

        // There should never be a ghost
        VERIFY_IS_TRUE(false);
    });

    VERIFY_IS_TRUE(error.IsSuccess());
    ManualResetEvent waiter;
    waiter.Wait(TimeSpan::FromSeconds(5));
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);
    VerifyCounts(ipam, 4, 0, 4);

    // Cancel refresh processing
    ipam->CancelRefreshProcessing();
}


BOOST_AUTO_TEST_CASE(SimpleGhostTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    AsyncOperationSPtr parent = root->CreateAsyncOperationRoot();
    vector<string> data;
    data.push_back(testData);
    data.push_back(testData_missing_10_0_0_6);

    const wchar_t *names[] = { L"Reservation1", L"Reservation2", L"Reservation3", L"Reservation4", L"Reservation5" };
    uint ips[5];
    bool ghostHit = false;

    IHttpClientSPtr http = make_shared<MockHttpClient>(
        *HttpConstants::HttpGetVerb,
        data,
        true,
        200,
        200,
        ErrorCodeValue::Success,
        ErrorCodeValue::Success);
    auto ipam = new IPAMWindows(http, root);

    expectSync_ = true;
    expectSuccess_ = true;

    auto operation = ipam->BeginInitialize(
        TimeSpan::FromMilliseconds(100),
        [this, ipam](AsyncOperationSPtr const &operation)
    {
        this->OnInitializeComplete(ipam, operation, false);
    },
        parent);

    this->OnInitializeComplete(ipam, operation, true);

    VerifyCounts(ipam, 5, 0, 5);

    // Use them all up
    //
    for (int i = 0; i < 5; i++)
    {
        ipam->Reserve(names[i], ips[i]);
    }
    VerifyCounts(ipam, 5, 5, 0);

    // Now start the recurring refresh
    //
    auto lastLoad = ipam->LastRefreshTime;
    auto error = ipam->StartRefreshProcessing(TimeSpan::FromSeconds(2), TimeSpan::FromMilliseconds(100),
        [&](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);
        ghostHit = true;
    });

    VERIFY_IS_TRUE(error.IsSuccess());

    ManualResetEvent waiter;
    waiter.Wait(TimeSpan::FromSeconds(5));
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);
    VERIFY_ARE_EQUAL(ipam->GhostCount, 1);
    auto ghost = ipam->GetGhostReservations();
    VERIFY_ARE_EQUAL((int)ghost.size(), 1);
    auto ghostName = ghost.front();

    int ghostIndex = -1;
    for (int i = 0; i < 5; i++)
    {
        if (ghostName == names[i])
        {
            ghostIndex = i;
        }
    }

    VERIFY_ARE_NOT_EQUAL(ghostIndex, -1);
    VerifyCounts(ipam, 4, 4, 0);

    // Release the reservation and see that the ghost disappears.
    //
    error = ipam->Release(names[ghostIndex]);
    VERIFY_IS_TRUE(error.IsSuccess());

    VERIFY_ARE_EQUAL(ipam->GhostCount, 0);
    ghost = ipam->GetGhostReservations();
    VERIFY_ARE_EQUAL((int)ghost.size(), 0);
    VerifyCounts(ipam, 4, 4, 0);

    // And finally, release all the reservations and make sure that they
    // work.
    //
    for (int i = 0; i < 5; i++)
    {
        if (i != ghostIndex)
        {
            error = ipam->Release(names[i]);
            VERIFY_IS_TRUE(error.IsSuccess());
        }
    }

    VerifyCounts(ipam, 4, 0, 4);

    // Cancel refresh processing
    ipam->CancelRefreshProcessing();
}

BOOST_AUTO_TEST_SUITE_END()

bool IPAMTest::Setup()
{
    return true;
}

bool IPAMTest::Cleanup()
{
    return true;
}

void IPAMTest::VerifyCounts(IPAM *ipam, int total, int inuse, int avail)
{
    VERIFY_ARE_EQUAL(total, ipam->TotalCount);
    VERIFY_ARE_EQUAL(inuse, ipam->ReservedCount);
    VERIFY_ARE_EQUAL(avail, ipam->FreeCount);
}

void IPAMTest::OnInitializeComplete(
    IPAM *ipam,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    VERIFY_ARE_EQUAL(operation->CompletedSynchronously, this->expectSync_);
    VERIFY_ARE_EQUAL(operation->Error.IsSuccess(), this->expectSuccess_);

    auto error = AsyncOperation::End(operation);
    VERIFY_ARE_EQUAL(error.IsSuccess(), this->expectSuccess_);
    VERIFY_ARE_EQUAL(ipam->Initialized, this->expectSuccess_);
}
#endif
