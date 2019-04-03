// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// Service Fabric block store service implementation.

// TODO: Review this.
// Silence warnings for stdenv
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "../inc/CDriverClient.h"
#include "../inc/CBlockStoreService.h"
#include "../inc/CBlockStoreRequest.h"
#include <shlobj.h>
#if !defined(PLATFORM_UNIX)
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#else
#include <curl/curl.h>
#include <fstream>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
typedef void (*_crt_signal_t) (int);
#endif

using namespace std;
using namespace Common;
using boost::asio::ip::tcp;
using namespace service_fabric;

// Define the global mutex used for synchronizing
// access to the service instance.
std::mutex CBlockStoreService::mutexSharedAccess_;

CBlockStoreService::CBlockStoreService(service_fabric::reliable_service &service, boost::asio::io_service& ioService, u16string serviceName, string servicePartitionId, unsigned short port, bool isSingleNodeCluster, CDriverClient *pDriverClient)
    :
    ComUnknownBase(),
    reliableService_(service),
#if defined(USE_BLOCKSTORE_SERVICE_ON_NODE)
    // Make service listen on 127.0.0.1 when running in local connection node.
    acceptor_(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(BlockStore::Constants::LocalHostIPV4Address), port)),
#else // !USE_BLOCKSTORE_SERVICE_ON_NODE
    // Make service listen on all IP addresses when not running in local connection mode
    acceptor_(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
#endif // USE_BLOCKSTORE_SERVICE_ON_NODE
    socket_(ioService),
    servicePartitionId_(servicePartitionId),
    serviceName_(serviceName),
    isSingleNodeCluster_(isSingleNodeCluster),
    driverClient_(pDriverClient),
    isCodePackageActive_(false),
    dependentCodepackageTerminateCount_(0),
    MAX_TERMINATE_THRESHOLD_(10),
    partition_(),
    fIsCodePackageActivationProcessed_(false),
    fIsCodePackageDeactivationProcessed_(false),
    hresultCodePackageActivation_(E_FAIL),
    hresultCodePackageDeactivation_(E_FAIL)
{
    // If port was specified as 0, it will be randomly assigned by the socket
    // subsystem. Thus, update the port value.
    servicePort_ = acceptor_.local_endpoint().port();

    // By default, our replica role is undefined.
    lastReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;

    // Init our reference
    get_Instance() = this;

    // By default, ChangeRole has not completed
    fIsChangeRoleComplete_ = false;

    // By default, service is not ready to process any requests
    fIsServiceReady_ = false;
    fRefreshLUListDuringStartup_ = false;

    serviceInterrupted_ = false;

    HRESULT hr = FabricGetCodePackageActivator(
        IID_IFabricCodePackageActivator,
        codePackageActivator_.VoidInitializationAddress());


    if (FAILED(hr))
    {
        TRACE_ERROR("Unable to Get FabricGetCodePackageActivator");
    }
}

CBlockStoreService::~CBlockStoreService()
{    
}

void CBlockStoreService::Cleanup()
{
    // Take the mutex before performing any shutdown activity
    std::lock_guard<std::mutex> lock{ CBlockStoreService::mutexSharedAccess_ };

    // Shutdown any associated volumes if the service is terminating.
    ShutdownVolumes(true);

    TriggerServiceShutdown();

    // Mark the service instance as free
    get_Instance() = nullptr;
}


// Return empty string when fail to get port
string CBlockStoreService::GetDockerVolumeDriverPort()
{
    string port = "";
    char fileBuffer[BlockStore::Constants::MaxBytesRead];

    // Fetch Local ProgramData folder path.
    wchar_t* localAppData = 0;

#ifdef PLATFORM_UNIX
    wstring fabricDataRoot;
    auto error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
    if (!error.IsSuccess())
    {
        TRACE_ERROR(L"GetFabricDataRoot failed");
        return "";
    }

    wstring wfileName(fabricDataRoot);
    wfileName += L"/";
    wfileName += BlockStore::Constants::DockerPluginFilenameLinux;

    string fileName;
    StringUtility::UnicodeToAnsi(wfileName, fileName);

    FILE *pFile = fopen(fileName.c_str(), "r");
#else
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, NULL, &localAppData);

    if (!SUCCEEDED(hr))
    {
        TRACE_ERROR("Fetch Local ProgramData folder path error.");
        return "";
    }
    wstring fileName(localAppData);

    fileName += BlockStore::Constants::DockerPluginFilename;

    CoTaskMemFree(static_cast<void*>(localAppData));

    FILE * pFile = _wfopen(fileName.c_str(), L"r");
#endif
    if (pFile != NULL)
    {
        if (fgets(fileBuffer, BlockStore::Constants::MaxBytesRead, pFile) != NULL)
        {
            // File content format and sample
            // "{{\"Name\":\"bsdocker\", \"Addr\": \"http://{0}:{1}\"}}"
            // {"Name":"bsdocker", "Addr": "http://localhost:19007"}
            string fileString(fileBuffer);

            auto startPos = fileString.find_last_of(":");
            auto endPos = fileString.find_last_of("\"");
            if (startPos != string::npos && endPos != string::npos && startPos < endPos)
            {
                port = fileString.substr(startPos + 1, endPos - startPos - 1);
            }
            else
            {
                TRACE_ERROR("File content upexpected.");
            }
        }
        else
        {
            TRACE_ERROR("File read failed.");
        }
        fclose(pFile);
    }
    else 
    {   
        TRACE_ERROR("File open failed.");
    }
    return port;
}

// Invoke to register with Volume Driver when service changes role to primary.
// Caller should ensure we are in Primary role.
bool CBlockStoreService::RegisterWithVolumeDriverWhenPrimary()
{
#if defined(RELIABLE_COLLECTION_TEST)
    // Ignore the error during registeration with Volume driver
    // for development scenario build.
    TRACE_WARNING("Ignoring registration with Volume Driver for standalone test.");
    return true;
#else
    bool registerSuccessful = RegisterWithVolumeDriver();

    if (registerSuccessful)
    {
        TRACE_INFO("Service successfully registered with Volume Driver.");
    }
    else
    {
        TRACE_ERROR("Service was unable to register with Volume Driver.");
    }

    return registerSuccessful;
#endif
}

#ifdef PLATFORM_UNIX

struct PostWriteData
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    volatile int result;
};

size_t PostWriteCB(char *ptr, size_t size, size_t nitems, void *args)
{
    struct PostWriteData *data = (struct PostWriteData *)args;
    int reslen = strlen(BlockStore::Constants::DockerRegisterSuccessful);

    pthread_mutex_lock(&data->mutex);
    if(size*nitems >= reslen && 
        (strncmp(ptr, BlockStore::Constants::DockerRegisterSuccessful, reslen) == 0))
    {
        //success.
        data->result = 0;
    }
    else
    {
        //error.
        data->result = 1;
    }
    pthread_mutex_unlock(&data->mutex);
    pthread_cond_signal(&data->cond);
    return size*nitems;
}

bool CBlockStoreService::RegisterWithVolumeDriverWorker()
{
    bool fRetVal = false;
    //namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
    CURL *curl;
    CURLcode res;
    string url = "";
    string postfields = ""; 
    struct PostWriteData data;
    struct timespec ts= {0, 0};
    struct timeval tc;
    int rc;

    try
    {
        const string dockerVolumeDriverHost = "localhost";
        const string dockerVolumeDriverPort = GetDockerVolumeDriverPort();
        const string dockerVolumeDriverTarget = "/VolumeDriver.BlockStoreRegister";

        if (dockerVolumeDriverPort == "")
        {
            TRACE_ERROR("[RegisterWithVolumeDriverWorker] Unable to fetch VolumeDriver endpoint.");
            return fRetVal;
        }

        //get curl handle.
        curl = curl_easy_init();
        if(!curl)
        {
            TRACE_ERROR("[RegisterWithVolumeDriverWorker] Unable to create curl handle!");
            return fRetVal;
        }

        url += dockerVolumeDriverHost + ":" + dockerVolumeDriverPort + dockerVolumeDriverTarget;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        postfields = servicePartitionId_ + " " + to_string(servicePort_);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postfields.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

        //Set write callback to parse post response.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, PostWriteCB);

        //setup data for write callback.
        memset(&data, 0, sizeof(struct PostWriteData));
        pthread_cond_init(&data.cond, NULL);
        pthread_mutex_init(&data.mutex, NULL);
        data.result = -1;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

        //Wait on writecbdata to get the result.
        gettimeofday(&tc, NULL);
        /* Convert from timeval to timespec */
        ts.tv_sec  = tc.tv_sec;
        ts.tv_nsec = tc.tv_usec * 1000;
        //Wait for 30 seconds before marking request timedout.
        ts.tv_sec += 30;

        //Send registration request.
        curl_easy_perform(curl);

        pthread_mutex_lock(&data.mutex);
        if(data.result == -1){
            rc = pthread_cond_timedwait(&data.cond, &data.mutex, &ts);
            if (rc != 0) {
                TRACE_ERROR("[RegisterWithVolumeDriverWorker] Service registration failed with errorcode " << rc);
                pthread_mutex_unlock(&data.mutex);
                curl_easy_cleanup(curl);
                return fRetVal;
            }
        }
        pthread_mutex_unlock(&data.mutex);

        //Check result.
        if (data.result == 0)
        {
            fRetVal = true;
        }
        else
        {
            fRetVal = false;
            TRACE_ERROR("[RegisterWithVolumeDriverWorker] Service registration failed. Volume Driver did not return \"RegisterSuccessful\". " << res);
        }
    }
    catch (std::exception const& e)
    {
        fRetVal = false;
        TRACE_ERROR("[RegisterWithVolumeDriverWorker] Service registration failed. Error: " << e.what());
    }

    curl_easy_cleanup(curl);
    return fRetVal;
}
#else
// Register Block Store Service (Name, Port) to Docker Volume Driver
bool CBlockStoreService::RegisterWithVolumeDriverWorker()
{
    bool fRetVal = false;
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
    namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // These objects perform our I/O
    tcp::resolver resolver{ ioc };
    tcp::socket socket{ ioc };

    bool fShutdownSocket = false;

    try
    {
        const string dockerVolumeDriverHost = "localhost";
        const string dockerVolumeDriverPort = GetDockerVolumeDriverPort();
        const string dockerVolumeDriverTarget = "/VolumeDriver.BlockStoreRegister";

        if (dockerVolumeDriverPort == "")
        {
            TRACE_ERROR("Unable to fetch VolumeDriver endpoint.");
            return fRetVal;
        }

        TRACE_INFO("VolumeDriver endpoint: " << dockerVolumeDriverPort);

        // Look up the domain name
        // A successful call to this function is guaranteed to return a non-empty range
        // Exceptions
        //  boost::system::system_error 
        //      Thrown on failure.
        auto const results = resolver.resolve(dockerVolumeDriverHost, dockerVolumeDriverPort);

        // Make the connection on the IP address we get from a lookup
        // Exceptions
        //  boost::system::system_error 
        //      Thrown on failure.If the sequence is empty, the associated error_code is boost::asio::error::not_found.Otherwise, contains the error from the last connection attempt.
        boost::asio::connect(socket, results.begin(), results.end());
        fShutdownSocket = true;

        http::request<http::string_body> req{ http::verb::post, dockerVolumeDriverTarget, BlockStore::Constants::DefaultHTTPVersion };

        // Set up an HTTP POST request message with the following format:
        //
        // <service_partition_id> <service_port>
        req.set(http::field::host, dockerVolumeDriverHost);
        req.body() = servicePartitionId_ + " " + to_string(servicePort_);
        req.set(http::field::content_length, req.body().size());

        // Send the HTTP request to the remote host
        // This function is used to write a complete message to a stream using HTTP/1. The call will block until one of the following conditions is true: 
        //      The entire message is written.
        //      An error occurs.
        // Exceptions
        //  boost::system::system_error 
        //      Thrown on failure.
        http::write(socket, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        // This function is used to read a complete message from a stream using HTTP/1. The call will block until one of the following conditions is true: 
        //      The entire message is read.
        //      An error occurs.
        // Exceptions
        //  boost::system::system_error 
        //      Thrown on failure.
        http::read(socket, buffer, res);

        // Gracefully close the socket
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        fShutdownSocket = false;

        // not_connected happens sometimes
        // so don't bother reporting it.
        if (ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ ec };

        // If we get here then the connection is closed gracefully
        // And write and read are successful

        // Need to check whether the Volume Driver return "RegisterSuccessful";
        if (boost::beast::buffers_to_string(res.body().data()) == BlockStore::Constants::DockerRegisterSuccessful)
        {
            fRetVal = true;
        }
        else
        {
            fRetVal = false;
            TRACE_ERROR("Service registration failed. Volume Driver did not return \"RegisterSuccessful\". " << res);
        }
    }
    catch (std::exception const& e)
    {
        fRetVal = false;
        TRACE_ERROR("Service registration failed. Error: " << e.what());
    }

    if (fShutdownSocket)
    {
        try
        {
            boost::system::error_code ec;
            socket.shutdown(tcp::socket::shutdown_both, ec);
        }
        catch (std::exception const& e)
        {
            TRACE_ERROR("Unable to shutdown the connection with VolumeDriver. Error: " << e.what());
        }
    }

    return fRetVal;
}
#endif

// Register Block Store Service (Name, Port) to Docker Volume Driver
bool CBlockStoreService::RegisterWithVolumeDriver()
{
    bool fRegistered = false;

    for(uint8_t iRetryCount = 0; iRetryCount < BlockStore::Constants::VolumeDriverRegistrationRetryCount; iRetryCount++)
    {
        fRegistered = RegisterWithVolumeDriverWorker();
        if (fRegistered)
        {
            // We successfully registered with the volume driver, so break out of the loop.
            break;
        }
        else
        {
            // Volume Driver service may not be up, so wait out before retrying
            TRACE_INFO("Attempt #" << (iRetryCount+1) << " failed to connect with Volume Driver service. Will retry in " << BlockStore::Constants::VolumeDriverRegistrationRetryIntervalInSeconds << " seconds.");
            if (iRetryCount < (BlockStore::Constants::VolumeDriverRegistrationRetryCount -1))
            {
                std::this_thread::sleep_for (std::chrono::seconds(BlockStore::Constants::VolumeDriverRegistrationRetryIntervalInSeconds));
            }
        }
    }

    return fRegistered;
}

// Helper to initiate the service shutdown
void CBlockStoreService::TriggerServiceShutdown()
{
    if (!serviceInterrupted_)
    {
        if (!threadPool_.empty())
        {
            TRACE_INFO("Interrupting ThreadPool threads.");
            for (auto thread : threadPool_)
                thread->interrupt();
        }
        else
        {
            TRACE_INFO("No ThreadPool threads available.");
        }

        TRACE_INFO("Stopping the IO Service.");
        acceptor_.get_io_service().stop();
        serviceInterrupted_ = true;
    }
}

#if defined(PLATFORM_UNIX)
void CBlockStoreService::InterruptHandlerCallback(
                    int sig, siginfo_t *siginfo, void *context) 
#else
void CBlockStoreService::InterruptHandlerCallback(int param)
#endif
{
#if !defined(PLATFORM_UNIX)
    UNREFERENCED_PARAMETER(param);
#endif

    // If we are here, an interrupt signal came to our process.
    // In such a case, we will mark the threadPool threads as interrupted.
    // Since they check for interruption state, they will break out and exit,
    // resulting in ordered shutdown of the service.

    // Take the mutex before using the reference to service instance
    {
        // Scope the lock so that it gets released prior to us blocking (if applicable)
        std::lock_guard<std::mutex> lockServiceInstance{CBlockStoreService::mutexSharedAccess_};

        CBlockStoreService *pService = CBlockStoreService::get_Instance();
        if (pService != nullptr)
        {
            // Trigger the service shutdown
            TRACE_INFO("Triggering service shutdown.");
            pService->TriggerServiceShutdown();
        }
        else
        {
            TRACE_INFO("No service instance available.");
        }
    }
}

#ifdef PLATFORM_UNIX
static void linux_sig_handler(int sig, siginfo_t *siginfo, void *context) 
{
    // get pid of sender,
    pid_t sender_pid = siginfo->si_pid;
    TRACE_INFO("Got signal:" << sig << " from process:" << sender_pid);
    return;
}
#endif

bool CBlockStoreService::InitializeInterruptHandlers()
{
    TRACE_INFO("Initializing interrupt handler.");
#ifdef PLATFORM_UNIX
    siga.sa_sigaction = CBlockStoreService::InterruptHandlerCallback;
    siga.sa_flags |= SA_SIGINFO; // get detail info

    // change signal action,
    if(sigaction(SIGINT, &siga, NULL) != 0) {
        TRACE_ERROR("sigaction failed with error " << errno);    
        return false;
    }
#else
    if (signal(SIGINT, (_crt_signal_t)&CBlockStoreService::InterruptHandlerCallback) == SIG_ERR)
    {
        TRACE_ERROR("Unable to initialize interrupt handler.");    
        return false;
    }
#endif
    return true;
}

// Helper to wait for Reliable Collection Services (RCS) infrastructure to get ready
//
// TODO: Remove this once the underlying infrastructure delivers the same capability
//       via a notification.
bool CBlockStoreService::IsRCSReady()
{
    bool fReady = false;

    TRACE_INFO("Waiting for Reliable Collection Service infrastructure to get ready.");
            
    for(uint8_t iRetryCount = 0; iRetryCount < BlockStore::Constants::RCSReadyRetryCount; iRetryCount++)
    {
        HRESULT status = E_FAIL;
        try
        {
            // When RCS is ready, we will be able to get the list of maps in it (even if the count is zero)
            auto context = reliableService_.get_context(0);
            
            bool fMapExists = false;

            // Check to see if can query for a non-existent map
            context.get_reliable_map_if_exists<u16string, vector<BlockStoreByte>>(u"NonExistentMap", &fMapExists, &status);
            if (fMapExists)
            {
                // If map exists, it also implies that RCS infrastructure is ready!
                fReady = true;
            }
            else if (status == FABRIC_E_NAME_DOES_NOT_EXIST)
            {
                // If the map does not exist and the return status is FABRIC_E_NAME_DOES_NOT_EXIST, we know the RCS infrastructure
                // if working.
                fReady = true;
            }

        }
        catch(reliable_exception& ex)
        {
            // Exception implies RCS is not ready or we ran into some issue. So swallow it and retry if applicable.
            status = ex.code();
        }

        if (!fReady)
        {
            TRACE_WARNING("Attempt #" << (iRetryCount+1) << " - Reliable Collection Service infrastructure not ready due to HRESULT " << status << ". Will retry in " << BlockStore::Constants::RCSReadyRetryIntervalInSeconds << " seconds.");
            if (iRetryCount < (BlockStore::Constants::RCSReadyRetryCount -1))
            {
                std::this_thread::sleep_for (std::chrono::seconds(BlockStore::Constants::RCSReadyRetryIntervalInSeconds));
            }
        }
        else
        {
            TRACE_INFO("Reliable Collection Service infrastructure is ready.");
            break;
        }
    }

    return fReady;
}

// Start the service by having the listeners up.
bool CBlockStoreService::Start()
{
    bool fServiceStarted = false;

    // Setup the handlers for Ctrl+C/Ctrl+Break
    if(InitializeInterruptHandlers() != true)
    {
        TRACE_ERROR("InitializeInterruptHandlers failed!");
        return false;        
    }

#ifndef RELIABLE_COLLECTION_TEST
    // Was code package initialization successful?
    if (!codePackageActivator_.GetRawPointer())
    {
        return false;
    }
#endif
    // Scope the lock
    {
        // Wait for ChangeRole to complete
        TRACE_INFO("Waiting for ChangeRole notification to complete.");
        std::unique_lock<std::mutex> lockChangeRoleComplete(ChangeRoleCompleteMutex);
        cvChangeRoleComplete_.wait(lockChangeRoleComplete, [this]{return (bool)fIsChangeRoleComplete_;});
        TRACE_INFO("ChangeRole notification completed.");
    }

#ifdef PLATFORM_UNIX
    //Initialize curl library exactly once.
    curl_global_init(CURL_GLOBAL_ALL);
#endif

    // Register with the VolumeDriver if we are executing as primary
    //
    // TODO: Fix the variable name here
    if (RefreshLUListDuringStartup)
    // Now that ChangeRole is completed, wait for RCS to get ready
    {
        // Fail to continue if registration fails.
        if (!RegisterWithVolumeDriverWhenPrimary())
        {
            return false;
        }
    }
    else
    {
        TRACE_INFO("Will register with Volume Driver on change role to Primary.");
    }

    TRACE_INFO("Listening on " << acceptor_.local_endpoint());

    // Setup the listener for incoming requests
    AcceptNextIncomingRequest();

    // Mark the service as ready to use
    IsServiceReady = true;

    int thread_count = std::max(boost::thread::hardware_concurrency(), 2u);
    TRACE_INFO("Firing " << thread_count << " threads.");

    // Create a pool of threads to run all of the io_services.
    for (int i = 0; i < thread_count; ++i)
    {
        // Setup the thread to process IOService event loop.
        std::shared_ptr<boost::thread> thread_ptr = nullptr;
        try
        {
            thread_ptr = std::make_shared<boost::thread>([&] {
                try
                {
                    // Continue to process requests until we were interrupted.
                    while (!boost::this_thread::interruption_requested())
                    {
                        acceptor_.get_io_service().run_one();
                    }
                }
                catch (boost::thread_interrupted&) 
                {
                    TRACE_INFO("Thread '" << boost::this_thread::get_id() << "' interrupted.");    
                }
                catch (std::exception &ex)
                {
                    TRACE_ERROR("Thread '" << boost::this_thread::get_id() << "' exception: " << ex.what());
                }

                TRACE_INFO("Thread '" << boost::this_thread::get_id() << "' exiting.");
            });
        }
        catch (std::exception &ex)
        {
            TRACE_ERROR("ThreadPool thread could not be created due to exception: " << ex.what());
            thread_ptr = nullptr;
        }

        if (thread_ptr != nullptr)
        {
            threadPool_.push_back(thread_ptr);
            TRACE_INFO("Thread " << i << " is ready.");
        }
    }

    if (!threadPool_.empty())
    {
        // Refresh the LU List
        if (RefreshLUListDuringStartup)
        {
            // Primary replica comes here
            if (ActivateVolumesAndCodePackages((KEY_TYPE)-1, FABRIC_REPLICA_ROLE_PRIMARY))
            {
                TRACE_INFO("Activated volumes and dependent code packages during startup.");
                fServiceStarted = true;
            }
            else
            {
                TRACE_WARNING("Unable to activate volumes and/or dependent code packages during startup.");    
            }
        }
        else
        {
            // Secondary replica comes here
            // and will be waiting on the threads.
            fServiceStarted = true;
        }

        if (fServiceStarted)
        {
            TRACE_INFO("Waiting for work on ThreadPool threads.");
            for (auto thread : threadPool_)
                thread->join();
        }
        else
        {
            TRACE_WARNING("Service failed to initialize during startup.");  
        }
    }
    else
    {
        TRACE_ERROR("No ThreadPool threads could be initialized for processing the incoming requests.");
    }

    return fServiceStarted;
}

// Connects to the driver to refresh the LU list, in the storage stack, maintained with this service instance.
bool CBlockStoreService::RefreshLUList()
{
    // Now that we have service working threads up, ask the driver to refresh the LU list.
    // Ask the driver to refresh the LU List
    bool fRefreshedLUList = false;
    if (driverClient_)
    {
        TRACE_INFO("RefreshLUList: Connecting to the driver to refresh volume list");
        fRefreshedLUList = driverClient_->RefreshServiceLUList(servicePort_);
    } 
    else
    {
        TRACE_ERROR("RefreshLUList: Failed to initialize driverClient.");
    }

    if (!fRefreshedLUList)
    {
        TRACE_ERROR("RefreshLUList: Failed to refresh the LU list.");
#if defined(DBG) || defined(_DEBUG)
        // On debug builds, ignore the failure to refresh LU List
        // as we may be running the service (for testing/development) on a machine
        // different from where the driver is installed.

        TRACE_WARNING("RefreshLUList: Ignoring LU list refresh failure.");
        fRefreshedLUList = true;
#endif // defined(DBG) || defined(_DEBUG)
    }
    else
    {
        TRACE_INFO("RefreshLUList: Successfully refreshed the LU list.");       
    }
    return fRefreshedLUList;
}

// Connects to the driver to shutdown the volumes associated with the service.
bool CBlockStoreService::ShutdownVolumes(bool fServiceShuttingDown)
{
    if (fServiceShuttingDown)
    {
        TRACE_INFO("Shutting down volumes during service shutdown.")
    }

    bool fShutdownVolumes = false;
    if (driverClient_)
    {
        fShutdownVolumes = driverClient_->ShutdownVolumesForService(servicePort_);
    }

    if (!fShutdownVolumes)
    {
        TRACE_ERROR("ShutdownVolumes: Failed to shutdown service volumes.");
    }
    else
    {
        TRACE_INFO("ShutdownVolumes: Successfully shutdown service volumes.");
    }

    return fShutdownVolumes;
}

// Sets up the handler that will be invoked when the socket receives a request from the client (Driver).
void CBlockStoreService::AcceptNextIncomingRequest()
{
    std::shared_ptr<CBlockStoreRequest> incomingRequest = nullptr;
    
    // TODO: We should have a retry count and fail if that is crossed.
    while (incomingRequest == nullptr)
    {
        try
        {
            // Create a new BlockStoreRequest instance to handle the incoming request
            incomingRequest = std::make_shared<CBlockStoreRequest>(reliableService_, acceptor_.get_io_service(), driverClient_, servicePort_);
        }
        catch (std::exception &ex)
        {
            TRACE_ERROR("Insufficient memory to handle incoming request. Exception: " << ex.what());
            incomingRequest = nullptr;
        }
    }
    
    // Make the acceptor accept connection on the request's socket
    acceptor_.async_accept(incomingRequest->Socket,
        [incomingRequest, this](boost::system::error_code ec)
    {
        if (ec)
        {
            TRACE_INFO("Accept error: " << ec.message());

            // CloseSocket
            incomingRequest->Socket.close();
        }
        else
        {
            TRACE_NOISE("Connection accepted...");

            try
            {
                // Process the incoming request
                TRACE_NOISE("Processing Request for source port:" << incomingRequest->Socket.local_endpoint().port() << " destination port:" << incomingRequest->Socket.remote_endpoint().port());

                incomingRequest->ProcessRequest();
            }
            catch (std::exception& ex)
            {
                TRACE_ERROR("Exception during request dispatch " << ex.what());

                // CloseSocket
                incomingRequest->Socket.close();
            }
        }

        // Loop to create the next request handler
        AcceptNextIncomingRequest();
    });
}

extern CBlockStoreService *pGlobalServiceInstance;

bool CBlockStoreService::ActivateVolumesAndCodePackages(KEY_TYPE key, FABRIC_REPLICA_ROLE newRole)
{
    bool fRefreshedLUList = false;
    wstring healthProperty = L"SFVolumeDiskRefresh";
    wstring healthDescription = L"Unable to refresh volumes";

    if (IsRCSReady())
    {
        if (RefreshLUList())
        {
            if (SUCCEEDED(ActivateDependentCodePackages()))
            {
                fRefreshedLUList = true;
            }
            else
            {
                healthProperty = L"SFVolumeDiskDependentCodePackages";
                healthDescription = L"Unable to activate dependent code packages";
                TRACE_ERROR("Failed to activate dependent code packages.");    
            }
        }
        else
        {
            TRACE_ERROR("Dependent code packages will not be activated as volume list refresh failed.");
        }
    }
    else
    {
        // Trigger the service shutdown
        TRACE_WARNING("Unable to refresh volume list since RCS was not ready. Key = " << key << ", newRole = " << newRole);
    }

    if (!fRefreshedLUList)
    {
        if (partition_.GetRawPointer())
        {
            FABRIC_HEALTH_INFORMATION healthInfo = {0};
            healthInfo.SourceId = L"SFVolumeDiskService";
            healthInfo.Property = healthProperty.c_str();
            healthInfo.TimeToLiveSeconds = 60;
            healthInfo.Description = healthDescription.c_str();
            healthInfo.State = FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR;
            partition_->ReportPartitionHealth(&healthInfo);
            partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        }
    }
    else
    {
        TRACE_INFO("Volumes and Dependent code packages successfully activated.");
    }

    return fRefreshedLUList;
}


void STDMETHODCALLTYPE CBlockStoreService::OnCodePackageEvent(
    /* [in] */ IFabricCodePackageActivator *source,
    /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc)
{
    if (source == codePackageActivator_.GetRawPointer())// if it was send for this activator
    {
        TRACE_INFO("OnCodePackageEvent from hosting");
        if (eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_START_FAILED ||
            eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_STOPPED ||
            eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_TERMINATED)
        {
            //take action on the failed
            if (eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_START_FAILED)
            {
                TRACE_INFO("OnCodePackageEvent   FABRIC_CODE_PACKAGE_EVENT_TYPE_START_FAILED  for codepackage = " << eventDesc->CodePackageName);
                dependentCodepackageTerminateCount_++;
            }
            
            if (eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_STOPPED)
            {
                TRACE_INFO("OnCodePackageEvent   FABRIC_CODE_PACKAGE_EVENT_TYPE_STOPPED for codepackage = " << eventDesc->CodePackageName);
            }

            if (eventDesc->EventType == FABRIC_CODE_PACKAGE_EVENT_TYPE_TERMINATED)
            {
                TRACE_INFO("OnCodePackageEvent   FABRIC_CODE_PACKAGE_EVENT_TYPE_TERMINATED for codepackage = " << eventDesc->CodePackageName);
                dependentCodepackageTerminateCount_++;             
            }   
           
            if (dependentCodepackageTerminateCount_ > MAX_TERMINATE_THRESHOLD_)
            {
                TRACE_ERROR("Application has terminated more than expected, Requesting RA to move replica for codepackage = " << eventDesc->CodePackageName);
                // Request RA to move by inducing transiant fault.

                if (partition_.GetRawPointer())
                {
                    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
                }
                //reset terminate count, so we don't keep asking RA for replica fault.
                dependentCodepackageTerminateCount_ = 0;
            }
        }
    }
}

void STDMETHODCALLTYPE CBlockStoreService::Invoke(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (deactivateContext_.GetRawPointer() == context)
    { //deactivate callback
        hresultCodePackageDeactivation_ = codePackageActivator_->EndDeactivateCodePackage(context);
        if (hresultCodePackageDeactivation_ == S_OK)
        {
            isCodePackageActive_ = false;
            TRACE_INFO("Dependent code packages deactivated");            
        }
        else
        {
            TRACE_ERROR("Hosting Failed to deactivate dependent code packages");           
        }

        // Flag code package deactivation as processed so that the waiter gets unblocked
        fIsCodePackageDeactivationProcessed_ = true;
        cvCodePackagesDeactivated_.notify_one();
        deactivateContext_.Release();
    }
    else if (activateContext_.GetRawPointer() == context)
    { //activate callback
        hresultCodePackageActivation_ = codePackageActivator_->EndActivateCodePackage(context);
        if (hresultCodePackageActivation_ == S_OK)
        {
            TRACE_INFO("Dependent code packages activated");
            isCodePackageActive_ = true;
        }
        else
        {
            TRACE_ERROR("Hosting Failed to activate dependent code packages");           
        }

        // Flag code package activation as processed so that the waiter gets unblocked
        fIsCodePackageActivationProcessed_ = true;
        cvCodePackagesActivated_.notify_one();
        activateContext_.Release();
    }
}

HRESULT CBlockStoreService::DeactivateDependentCodePackages()
{	
    HRESULT hr = E_FAIL;

    if (codePackageActivator_.GetRawPointer() && isCodePackageActive_ == true)
    {
        TRACE_INFO("Requesting deactivation of dependent code packages from hosting");
        codePackageActivator_->UnregisterCodePackageEventHandler(codePackageActivatorCallbackHandle_);
        FABRIC_STRING_LIST codePackageNames; codePackageNames.Count = 0;
        
        dependentCodepackageTerminateCount_ = 0;
        hr = codePackageActivator_->BeginDeactivateCodePackage(&codePackageNames,
                                                          0,
                                                          this, deactivateContext_.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            // Wait for code package deactivation processing to complete and get the result
            TRACE_INFO("Waiting for dependent code packages to deactivate.");
            std::mutex dummyMutex;
            std::unique_lock<std::mutex> lockDummyMutex(dummyMutex);
            cvCodePackagesDeactivated_.wait(lockDummyMutex, [this]{return (bool)fIsCodePackageDeactivationProcessed_;});
            hr = hresultCodePackageDeactivation_;

            // Reset the flags
            fIsCodePackageDeactivationProcessed_ = false;
            hresultCodePackageDeactivation_ = E_FAIL;
        }
        else
        {
            TRACE_ERROR("Unable to request deactivation of dependent code packages from hosting");
        }
    }

    TRACE_INFO("Dependent code package deactivation processed with HRESULT " << hr);

    return hr;
}

HRESULT CBlockStoreService::ActivateDependentCodePackages()
{
    HRESULT hr = E_FAIL;

    if (codePackageActivator_.GetRawPointer() && isCodePackageActive_ == false)
    {
        TRACE_INFO("Requesting activation of dependent code packages from hosting");
        codePackageActivator_->RegisterCodePackageEventHandler(this, &codePackageActivatorCallbackHandle_);
        FABRIC_STRING_LIST codePackageNames; codePackageNames.Count = 0;

        dependentCodepackageTerminateCount_ = 0;
        FABRIC_STRING_MAP environment;
        environment.Count = 0;
        FABRIC_APPLICATION_PARAMETER temp;
        std::wstring wstr;
        TxnReplicator_Info info;
        info.Size = sizeof(info);
        hr = reliableService_.get_replicator_info(replicator_, &info);
        if (SUCCEEDED(hr))
        {
            temp.Name = L"Fabric_Epoch";
            wstr = to_wstring(info.CurrentEpoch.DataLossNumber);
            wstr += L":";
            wstr += to_wstring(info.CurrentEpoch.ConfigurationNumber);
            temp.Value = wstr.c_str();
            environment.Items = &temp;
            environment.Count = 1;
        }
        else
        {
            TRACE_ERROR("Failed to get epoch");
            ASSERT_IF(FAILED(hr), "Epoch must be available from replicator");
        }

        if (SUCCEEDED(hr))
        {
            hr = codePackageActivator_->BeginActivateCodePackage(&codePackageNames,
                                                            &environment,
                                                            0,
                                                            this,
                                                            activateContext_.InitializationAddress());
            if (SUCCEEDED(hr))
            {
                // Wait for code package activation processing to complete and get the result
                TRACE_INFO("Waiting for dependent code packages to activate.");
                std::mutex dummyMutex;
                std::unique_lock<std::mutex> lockDummyMutex(dummyMutex);
                cvCodePackagesActivated_.wait(lockDummyMutex, [this]{return (bool)fIsCodePackageActivationProcessed_;});
                hr = hresultCodePackageActivation_;

                // Reset the flags
                fIsCodePackageActivationProcessed_ = false;
                hresultCodePackageActivation_ = E_FAIL;
            }
            else
            {
                TRACE_ERROR("Unable to request activation of dependent code packages from hosting");
            }
        }
    }

    TRACE_INFO("Dependent code package activation processed with HRESULT " << hr);

    return hr;
}

// This callback is invoked when replica is being closed and it can be invoked
// even when ChangeRole is not delivered.
void CBlockStoreService::RemovePartitionCallback(KEY_TYPE key)
{
    TRACE_INFO("RemovePartitionCallback: Key = " << key);

    // Take the mutex before using the reference to service instance
    std::lock_guard<std::mutex> lockServiceInstance{ CBlockStoreService::mutexSharedAccess_ };

    CBlockStoreService *pService = CBlockStoreService::get_Instance();
    if (pService != nullptr)
    {
        // We should shutdown the volumes associated with the current service
        // only if we were in a primary role.
        if (pService->lastReplicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            TRACE_INFO("RemovePartitionCallback: Deactivating dependent code packages and volumes. Key = " << key);
            pService->DeactivateCodePackagesAndShutdownVolumes(key);
            TRACE_INFO("RemovePartitionCallback: Processed deactivation of dependent code packages and volumes. Key = " << key);
        }
        else
        {
            TRACE_INFO("RemovePartitionCallback: Not deactivating dependent code packages and volumes since replica is not primary. Key = " << key);
        }
    }
}

void CBlockStoreService::AbortPartitionCallback(KEY_TYPE key)
{
    TRACE_INFO("AbortPartitionCallback: Key = " << key);
    // Take the mutex before using the reference to service instance
    std::lock_guard<std::mutex> lockServiceInstance{ CBlockStoreService::mutexSharedAccess_ };

    CBlockStoreService *pService = CBlockStoreService::get_Instance();
    if (pService != nullptr)
    {
        // We should shutdown the volumes associated with the current service
        // only if we were in a primary role.
        if (pService->lastReplicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            TRACE_INFO("RemovePartitionCallback: Deactivating dependent code packages and volumes. Key = " << key);
            pService->DeactivateCodePackagesAndShutdownVolumes(key);
            TRACE_INFO("RemovePartitionCallback: Processed deactivation of dependent code packages and volumes. Key = " << key);
        }
        else
        {
            TRACE_INFO("RemovePartitionCallback: Not deactivating dependent code packages and volumes since replica is not primary. Key = " << key);
        }
    }
}

void CBlockStoreService::AddPartitionCallback(KEY_TYPE key, TxnReplicatorHandle replicator, PartitionHandle partition)
{   
    UNREFERENCED_PARAMETER(key);
    // Scope the lock
    {        // Take the mutex before using the reference to service instance
        std::lock_guard<std::mutex> lockServiceInstance{ CBlockStoreService::mutexSharedAccess_ };
        CBlockStoreService *pService = CBlockStoreService::get_Instance();
        if (pService != nullptr)
        {
            pService->SetIFabricStatefulServicePartition(static_cast<IFabricStatefulServicePartition*>(partition));
            pService->replicator_ = replicator;
        }
    }  
}


void CBlockStoreService::SetIFabricStatefulServicePartition(IFabricStatefulServicePartition* partition)
{
    if (partition)
    {
        IFabricStatefulServicePartition2 * partition2;
        auto hr = partition->QueryInterface(&partition2);
        ASSERT_IF(FAILED(hr), "Partition must implement IFabricStatefulServicePartition2");
        partition_.SetAndAddRef(partition2);
    }
}

// This is invoked to deactivate dependent code packages and shutdown volumes associated with them.
void CBlockStoreService::DeactivateCodePackagesAndShutdownVolumes(KEY_TYPE key)
{
    TRACE_INFO("DeactivateCodePackagesAndShutdownVolumes: Deactivating code packages for key = " << key);
    if (SUCCEEDED(DeactivateDependentCodePackages()))
    {
        // We do not shutdown volumes on a single node cluster since there can be a race between a primary and secondary replica of a service
        // that could result in secondary shutting down the volume created by the primary since they will both operate on the same volume LUID.
        //
        // This is not a concern on a real cluster since multiple replicas of the same service are never placed on the same physical node and thus,
        // would never have this conflict.
        if (!IsSingleNodeCluster)
        {
            TRACE_INFO("DeactivateCodePackagesAndShutdownVolumes: Unmounting LU List as part of becoming secondary or at application removal. Key = " << key);
            ShutdownVolumes(false);
        }
        else
        {
            TRACE_INFO("DeactivateCodePackagesAndShutdownVolumes: Not unmounting LU List as part of becoming secondary or at application removal due to single node cluster mode. Key = " << key);    
        }
    }
    else
    {
        TRACE_ERROR("DeactivateCodePackagesAndShutdownVolumes: Unable to deactivate dependent code packages. Key = " << key);     
    }
}

// This callback is invoked when role is changed for the service.
void CBlockStoreService::ChangeRoleCallback(KEY_TYPE key, int32_t incomingRole)
{
    bool fActivateVolumesAndCodePackages = false;
    FABRIC_REPLICA_ROLE newRole = (FABRIC_REPLICA_ROLE)incomingRole;

    // Scope the lock
    {
        // Take the mutex before using the reference to service instance
        std::lock_guard<std::mutex> lockServiceInstance{CBlockStoreService::mutexSharedAccess_};

        CBlockStoreService *pService = CBlockStoreService::get_Instance();
        if (pService != nullptr)
        {
            // TODO: Use FabricReplicaRole enum
            TRACE_INFO("ChangeRoleCallback: Key = " << key << ", oldRole = " << pService->lastReplicaRole_ << ", newRole = " << newRole);

            // Process the requests only if the service is ready
            if (pService->IsServiceReady)
            {
                // If we switched to primary, then refresh the LU list.
                if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
                {
                    // TODO: Should we throw an exception here if the registration fails?
                    TRACE_INFO("ChangeRoleCallback: Registering with VolumeDriver as part of becoming primary. Key = " << key << ", newRole = " << newRole);
                    pService->RegisterWithVolumeDriverWhenPrimary();

                    fActivateVolumesAndCodePackages = true;
                }
                else
                {   // We should shutdown the volumes associated with the current service
                    // if we switched to a non-primary role from a primary role.
                    if (pService->lastReplicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
                    {
                        TRACE_INFO("ChangeRoleCallback: Resuming non-primary role change since dependent code packages have been shutdown.");
                        pService->DeactivateCodePackagesAndShutdownVolumes(key);
                    }
                }
            }
            else
            {
                TRACE_INFO("ChangeRoleCallback: Skipped role specific work since service has not yet initialized. Key = " << key << ", newRole = " << newRole);

                // Set the flag to refresh the LU List during startup if we were becoming primary.
                if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
                {
                    pService->RefreshLUListDuringStartup = true;
                    TRACE_INFO("ChangeRoleCallback: Will refresh LU list in startup path. Key = " << key << ", newRole = " << newRole);
                }
            }

            // Finally, update the lastReplicaRole
            pService->lastReplicaRole_ = newRole;

            // If our new Role is Primary or Secondary (Idle or Active),set the event to
            // unblock service startup if it is waiting.
            if (newRole >= FABRIC_REPLICA_ROLE_PRIMARY)
            {
                if (!pService->fIsChangeRoleComplete_)
                {
                    // Take the lock to indicate change role is ready
                    std::lock_guard<std::mutex> lockChangeRoleComplete{ pService->ChangeRoleCompleteMutex };

                    if (!pService->fIsChangeRoleComplete_)
                    {
                        // Mark ChangeRole as completed to unblock the startup of the service
                        pService->SetChangeRoleComplete();
                        TRACE_INFO("ChangeRoleCallback: Marked ChangeRole as completed. Key = " << key << ", newRole = " << newRole);
                    }
                }
            }
        }
        else
        {
            TRACE_INFO("ChangeRoleCallback: Skipping since service instance does not exist. Key = " << key << ", newRole = " << newRole);
        }
    }

    // If we have to activate volumes and code packages, it needs to be asynchronously to allow this ChangeRoleCallback to return so that
    // the partition becomes readable. Otherwise, accessing the reliable_map in the partition, which is done by IsRCSReady invoked by 
    // ActivateVolumesAndCodePackages, will timeout since this (ChangeRoleCallback) has not returned back to allow replica status to be updated.
    if (fActivateVolumesAndCodePackages)
    {
        TRACE_INFO("ChangeRoleCallback: Activating volumes and dependent code packages asynchronously. Key = " << key << ", newRole = " << newRole);
        std::thread asyncWork(CBlockStoreService::ActivateVolumesAndCodePackagesAsync, key, newRole);

        // Detach from the physical thread of execution so that it continues independent
        asyncWork.detach();
    }

    TRACE_INFO("ChangeRoleCallback: Processing complete for Key = " << key << ", newRole = " << newRole);
}

void CBlockStoreService::ActivateVolumesAndCodePackagesAsync(KEY_TYPE key, FABRIC_REPLICA_ROLE newRole)
{
    TRACE_INFO("ActivateVolumesAndCodePackagesAsync: Activating volumes and dependent code packages as part of becoming primary. Key = " << key << ", newRole = " << newRole);
 
    // Scope the lock
    {
        // Take the mutex before using the reference to service instance
        std::lock_guard<std::mutex> lockServiceInstance{CBlockStoreService::mutexSharedAccess_};

        CBlockStoreService *pService = CBlockStoreService::get_Instance();
        if (pService != nullptr)
        {
            pService->ActivateVolumesAndCodePackages(key, newRole);
        }
        else
        {
            TRACE_INFO("ActivateVolumesAndCodePackagesAsync: Skipping since service instance does not exist. Key = " << key << ", newRole = " << newRole);
        }
    }
}