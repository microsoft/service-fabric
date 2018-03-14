// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/rawptrstream.h"
#include "cpprest/filestream.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency;
using namespace utility;
using namespace ServiceModel;

StringLiteral const IPAMLinuxProvider("IPAM");

string const NMAgentDataUri("http://169.254.169.254:80/machine/plugins?comp=nmagent&type=getinterfaceinfov1");

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

class IPAMLinux::RetrieveNmAgentDataAyncOperation : public AsyncOperation
{
    DENY_COPY(RetrieveNmAgentDataAyncOperation)

public:
    RetrieveNmAgentDataAyncOperation(
        __in IPAMLinux & owner,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
          owner_(owner),
          timeout_(timeout)
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
        this->owner_.httpClient_->request(methods::GET).then([this, thisSPtr](pplx::task<http_response> task)
        {
            try
            {
                auto response = task.get();
                if (response.status_code() == web::http::status_codes::OK)
                {
                    auto respStr = response.extract_string().get();
                    wstring xml;
                    StringUtility::Utf8ToUtf16(respStr, xml);
                    try
                    {
                        FlatIPConfiguration config(xml);
                        this->owner_.OnNewIpamData(config);
                        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                    }
                    catch (XmlException e)
                    {
                        // Log that the processing didn't work, and return with a failure
                        //
                        WriteError(
                            IPAMLinuxProvider,
                            "Failed to process the incoming payload.  Error:{0}, Payload:\n{1}",
                            e.Error,
                            xml);
                        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                    }
                }
                else
                {
                    WriteError(
                        IPAMLinuxProvider,
                        "Failed to get status code OK from nm agent call.");
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                }
            }
            catch (std::exception const & e)
            {
                WriteError(
                    IPAMLinuxProvider,
                    "Failed to get data from nm agent, error {0}",
                    e.what());
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            }
        });
    }
      
private:
    // Holds a reference to the owning IPAM client instance
    //
    IPAMLinux & owner_;

    // Holds the timeout value used to limit the http requests
    //
    TimeSpan const timeout_;
};

class IPAMLinux::RefreshPoolAsyncOperation : public TimedAsyncOperation
{
    DENY_COPY(RefreshPoolAsyncOperation)

public:
    RefreshPoolAsyncOperation(
        IPAMLinux &owner,
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
    IPAMLinux & owner_;

    // Holds the timeout value used to limit the http requests
    //
    TimeSpan const httpTimeout_;
};

// --------------------------------------------------------------------------
// End async operation embedded helper classes
// --------------------------------------------------------------------------

IPAMLinux::IPAMLinux(ComponentRootSPtr const & root)
    : IPAM(root)
{
    auto httpclient = make_unique<http_client>(string_t(NMAgentDataUri));
    this->httpClient_ = move(httpclient);
}

IPAMLinux::~IPAMLinux()
{
}

AsyncOperationSPtr IPAMLinux::BeginInitialize(
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

ErrorCode IPAMLinux::EndInitialize(
    AsyncOperationSPtr const & operation)
{
    return RetrieveNmAgentDataAyncOperation::End(operation);
}

ErrorCode IPAMLinux::StartRefreshProcessing(
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
        IPAMLinuxProvider,
        "Periodic reservation pool refresh started, with an interval of {0}",
        refreshInterval);

    this->refreshInterval_ = refreshInterval;
    this->refreshTimeout_ = refreshTimeout;
    this->ghostChangeCallback_ = ghostChangeCallback;

    return InternalStartRefreshProcessing();
}

ErrorCode IPAMLinux::InternalStartRefreshProcessing()
{
    // if refresh processing has been cancelled, return.
    {
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
            IPAMLinuxProvider,
            "Failed to start a refresh processing pass, error {0}",
            refreshOperation_->Error);

        // if refresh operation failed, reset flag.
        {
            AcquireExclusiveLock lock(this->lock_);
            this->refreshInProgress_ = false;
        }
    }

    return this->refreshOperation_->Error;
}

void IPAMLinux::OnRefreshComplete(
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
            IPAMLinuxProvider,
            "Failure during refresh processing, error={0}",
            error);
    }

    // Refresh cycle is done, start a new one
    InternalStartRefreshProcessing();
}
