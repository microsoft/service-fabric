// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace HttpClient;
using namespace HttpCommon;

StringLiteral const IPAMWindowsProvider("IPAM");

wstring const NMAgentDataUri(L"http://169.254.169.254:80/machine/plugins?comp=nmagent&type=getinterfaceinfov1");

// --------------------------------------------------------------------------
// Begin async operation embedded helper classes
// --------------------------------------------------------------------------

// There are two helper classes used by the IPAM client:
// - RetrieveNmAgentDataAsyncOperation:  This class handles the communication
//   with NM Agent/wireserver to get the available IP data.
// 
// - RefreshPoolAsyncOperation:  This class manages the periodic refresh
//   operations that processes any background changes in the set of known IP
//   addresses.  This class uses RetrieveNmAgentDataAsyncOperation to get the
//   actual data.

class IPAMWindows::RetrieveNmAgentDataAyncOperation : public AsyncOperation
{
    DENY_COPY(RetrieveNmAgentDataAyncOperation)

public:
    RetrieveNmAgentDataAyncOperation(
        __in IPAMWindows & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
          owner_(owner),
          timeout_(timeout),
          clientRequest_(nullptr)
    {
    }

    virtual ~RetrieveNmAgentDataAyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RetrieveNmAgentDataAyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    // OnStart: Called to start the async operation.  Here we issue the GET
    // call, continuing once that completes.
    // 
    // The arguments are:
    //	thisSPtr: async operation that is now starting.
    //
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        clientRequest_ = nullptr;

        auto error = owner_.client_->CreateHttpRequest(
            NMAgentDataUri,
            timeout_,
            timeout_,
            timeout_,
            clientRequest_);

        if (!error.IsSuccess())
        {
            WriteError(
                IPAMWindowsProvider,
                "Failed to create httprequest for wireserver retrieval {0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        clientRequest_->SetVerb(*HttpConstants::HttpGetVerb);

        ByteBufferUPtr requestBody;

        auto operation = clientRequest_->BeginSendRequest(
            move(requestBody),
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnSendCompleted(operation, false);
            },
            thisSPtr);

        this->OnSendCompleted(operation, true);
    }

    // OnSendCompleted: This method is called when the http GET operation has
    // finished, and a response is available.  This method verifies that the
    // GET was successful, and that there's a valid response.  Assuming that it
    // is, this method then starts to retrieve the response message.  The
    // operations continue once that message retrieval has completed.
    //
    // The arguments are:
    //	operation: The current async operation context
    //  expectedCompletedSynchronously: true, if synchronous result; 
    //                                  false, if not.
    //
    void OnSendCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ULONG winHttpError;
        auto error = clientRequest_->EndSendRequest(operation, &winHttpError);

        if (!error.IsSuccess())
        {
            WriteWarning(
                IPAMWindowsProvider,
                "Failed to send the request to the wireserver, error {0}, http error {1}",
                error,
                winHttpError);

            TryComplete(operation->Parent, error);
            return;
        }
        
        USHORT statusCode;
        wstring description;
        auto err = clientRequest_->GetResponseStatusCode(&statusCode, description);

        if (!err.IsSuccess() || statusCode != 200)
        {
            WriteWarning(
                IPAMWindowsProvider,
                "Wireserver data request failed, error {0}, statuscode {1}, description {2}",
                err,
                statusCode,
                description);

            if (err.IsSuccess())
            {
                err = ErrorCode(ErrorCodeValue::OperationFailed);
            }

            TryComplete(operation->Parent, err);
            return;
        }

        auto response = clientRequest_->BeginGetResponseBody(
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnResponseCompleted(operation, false);
            },
            operation->Parent);

        this->OnResponseCompleted(response, true);
    }

    // OnResponseCompleted: The method is called when the response body is
    // ready for processing.  If it has been successfully delivered, the
    // contents are deserialized into a new configuration object, and handed
    // to the IPAM client for incorporation.
    // 
    // This is the terminating method in the retrieval sequence.
    //
    // The arguments are:
    //	operation: The current async operation context
    //  expectedCompletedSynchronously: true, if synchronous result; 
    //                                  false, if not.
    //
    void OnResponseCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto parent = operation->Parent;

        ByteBufferUPtr body;
        auto error = clientRequest_->EndGetResponseBody(operation, body);

        USHORT statusCode;
        wstring description;
        clientRequest_->GetResponseStatusCode(&statusCode, description);

        if (!error.IsSuccess() || statusCode != 200)
        {
            WriteWarning(
                IPAMWindowsProvider,
                "Wireserver data response failed, error {0}, statuscode {1}, description {2}",
                error,
                statusCode,
                description);

            if (error.IsSuccess())
            {
                error = ErrorCode(ErrorCodeValue::OperationFailed);
            }

            TryComplete(parent, error);
            return;
        }

        // Decode the payload from nm agent
        //
        string narrowXml((const char *)(body->data()), body->size() / sizeof(char));
        if (*narrowXml.rbegin() == '\0')
        {
            narrowXml.resize(narrowXml.size() - 1);
        }

        wstring xml = StringUtility::Utf8ToUtf16(narrowXml);
        try
        {
            FlatIPConfiguration config(xml);
            this->owner_.OnNewIpamData(config);
        }
        catch (XmlException e)
        {
            // Log that the processing didn't work, and return with a failure
            //
            WriteError(
                IPAMWindowsProvider,
                "Failed to process the incoming payload.  Error:{0}, Payload:\n{1}",
                e.Error,
                xml);

            TryComplete(parent, e.Error);
        }

        TryComplete(parent, ErrorCodeValue::Success);
    }

private:
    // Holds a reference to the owning IPAM client instance
    //
    IPAMWindows & owner_;

    // Holds the timeout value used to limit the http requests
    //
    TimeSpan const timeout_;

    // Holds the ongoing http request context
    //
    IHttpClientRequestSPtr clientRequest_;
};

class IPAMWindows::RefreshPoolAsyncOperation : public TimedAsyncOperation
{
    DENY_COPY(RefreshPoolAsyncOperation)

public:
    RefreshPoolAsyncOperation(
        IPAMWindows &owner,
        TimeSpan const httpTimeout,
        TimeSpan const refreshInterval,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(refreshInterval, callback, parent),
          owner_(owner),
          httpTimeout_(httpTimeout)
    {
    }

    virtual ~RefreshPoolAsyncOperation()
    {
    }

protected:
    // OnTimeout: This method is called when the timer expires.  At that point
    // it initiates a new cycle of retrieval from the wirserver, using the 
    // class above.
    // 
    // The arguments are:
    //	thisSPtr: async operation that is now starting.
    //
    void OnTimeout(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<RetrieveNmAgentDataAyncOperation>(
            owner_,
            httpTimeout_,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRetrievalComplete(operation, false);
            },
            thisSPtr);
    
        this->OnRetrievalComplete(operation, true);
    }

    // OnRetrievalComplete: This method is called when the refresh operation
    // against the wire server is done.  At that point, as this is a one shot
    // timer, this operation is terminated.
    // 
    // The arguments are:
    //	operation: The current async operation context
    //  expectedCompletedSynchronously: true, to process synchronous results; 
    //                                  false, to process asynchronous results.
    //
    void OnRetrievalComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        TryComplete(operation, ErrorCodeValue::Success);
    }

private:
    // Holds a reference to the owning IPAM client instance
    //
    IPAMWindows & owner_;

    // Holds the timeout value used to limit the http requests
    //
    TimeSpan const httpTimeout_;
};

// --------------------------------------------------------------------------
// End async operation embedded helper classes
// --------------------------------------------------------------------------

IPAMWindows::IPAMWindows(ComponentRootSPtr const & root)
    : IPAM(root)
{
    HttpClientImpl::CreateHttpClient(
        L"IPReservationClient",
        this->Root,
        client_);
}

IPAMWindows::IPAMWindows(
    IHttpClientSPtr &client,
    ComponentRootSPtr const & root)
    : IPAM(root),
    client_(client)
{
}

IPAMWindows::~IPAMWindows()
{
}

AsyncOperationSPtr IPAMWindows::BeginInitialize(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RetrieveNmAgentDataAyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode IPAMWindows::EndInitialize(
    AsyncOperationSPtr const & operation)
{
    return RetrieveNmAgentDataAyncOperation::End(operation);
}

ErrorCode IPAMWindows::StartRefreshProcessing(
    TimeSpan const refreshInterval,
    TimeSpan const refreshTimeout,
    function<void(DateTime)> ghostChangeCallback)
{
    // Check if ipam is initialized or refresh is in progress
    {
        AcquireExclusiveLock lock(this->lock_);
        if (!this->initialized_ || this->refreshInProgress_)
        {
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        this->refreshInProgress_ = true;
    }

    WriteInfo(
        IPAMWindowsProvider,
        "Periodic reservation pool refresh started, with an interval of {0}",
        refreshInterval);

    this->refreshInterval_ = refreshInterval;
    this->refreshTimeout_ = refreshTimeout;
    this->ghostChangeCallback_ = ghostChangeCallback;

    return InternalStartRefreshProcessing();
}

ErrorCode IPAMWindows::InternalStartRefreshProcessing()
{
    {
        // if refresh processing has been cancelled, return.
        AcquireExclusiveLock lock(this->lock_);

        if (this->refreshProcessingCancelled_)
        {
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        this->refreshOperation_ = AsyncOperation::CreateAndStart<RefreshPoolAsyncOperation>(
            *this,
            refreshTimeout_,
            refreshInterval_,
            [this](AsyncOperationSPtr const &operation)
        {
            // force callback execution on thread pool thread
            Threadpool::Post([this, operation]() { this->OnRefreshComplete(operation, false); });
        },
            this->Root.CreateAsyncOperationRoot());
    }

    this->OnRefreshComplete(this->refreshOperation_, true);

    if (!this->refreshOperation_->Error.IsSuccess())
    {
        // Log the failure as an error, and mark that the housekeeping
        // task will not proceed forward on its own.
        //
        WriteError(
            IPAMWindowsProvider,
            "Failed to start a refresh processing pass, error {0}",
            this->refreshOperation_->Error);

        // if refresh operation failed, reset flag.
        {
            AcquireExclusiveLock lock(this->lock_);
            this->refreshInProgress_ = false;
        }
    }

    return this->refreshOperation_->Error;
}

void IPAMWindows::OnRefreshComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = AsyncOperation::End(operation);
    if (!error.IsSuccess() && error.ReadValue() != ErrorCodeValue::Timeout)
    {
        WriteWarning(
            IPAMWindowsProvider,
            "Failure during refresh processing, error={0}",
            error);
    }

    // Refresh cycle is done, start a new one
    InternalStartRefreshProcessing();
}
