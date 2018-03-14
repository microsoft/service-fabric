// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace Naming;
using namespace TestCommon;

const StringLiteral TraceSource("FabricTest.TestClient");

std::wstring const TestStoreClient::InitialValue = L"";
FABRIC_SEQUENCE_NUMBER const TestStoreClient::InitialVersion = 0;
ULONGLONG const TestStoreClient::InitialDataLossVersion = 0;
int const TestStoreClient::RetryWaitMilliSeconds = 2000;

class TestStoreClient::TestClientEntry
{
public:
    TestClientEntry()
    {
    }

    TestClientEntry(Guid fuId, wstring const& value, int64 version, ITestStoreServiceWPtr && storeServiceWPtr, int64 dataLossVersion)
        : fuId_ (fuId), value_(value), version_(version), storeServiceWPtr_(move(storeServiceWPtr)), dataLossVersion_(dataLossVersion)
    {
    }

    __declspec (property(get=get_FUId)) Guid const& Id;
    Guid const& get_FUId() const {return fuId_;}

    __declspec (property(get=get_Value, put=put_Value)) wstring const& Value;
    wstring const& get_Value() const {return value_;}
    void put_Value(wstring const & value) { value_ = value; }

    __declspec (property(get=get_Version, put=put_Version)) FABRIC_SEQUENCE_NUMBER Version;
    FABRIC_SEQUENCE_NUMBER get_Version() const {return version_;}
    void put_Version(FABRIC_SEQUENCE_NUMBER const& version) { version_ = version; }

    __declspec (property(get=get_DataLossVersion, put=put_DataLossVersion)) ULONGLONG DataLossVersion;
    ULONGLONG get_DataLossVersion() const {return dataLossVersion_;}
    void put_DataLossVersion(ULONGLONG const& dataLossVersion) { dataLossVersion_ = dataLossVersion; }

    __declspec (property(get=get_StoreService, put=put_StoreService)) ITestStoreServiceWPtr const& StoreService;
    ITestStoreServiceWPtr const& get_StoreService() const {return storeServiceWPtr_;}
    void put_StoreService(ITestStoreServiceWPtr && storeServiceWPtr) { storeServiceWPtr_ = move(storeServiceWPtr); }

private:
    wstring value_;
    FABRIC_SEQUENCE_NUMBER version_;
    ITestStoreServiceWPtr storeServiceWPtr_;
    ULONGLONG dataLossVersion_;
    Guid fuId_;
};

class TestStoreClient::StoreServiceEntry
{
public:
    StoreServiceEntry(ITestStoreServiceSPtr && service)
        : service_(move(service))
        , root_()
    {
        // Create a ComponentRoot for debugging leaks via
        // reference tracking traces
        //
        auto casted = dynamic_cast<ComponentRoot*>(service_.get());
        if (casted)
        {
            root_ = casted->CreateComponentRoot();
        }
    }

    __declspec (property(get=get_Service, put=put_Service)) ITestStoreServiceSPtr const & StoreService;
    ITestStoreServiceSPtr const & get_Service() const { return service_; }

private:
    ITestStoreServiceSPtr service_;
    ComponentRootSPtr root_;
};

TestStoreClient::TestStoreClient(wstring const& serviceName)
    :serviceName_(serviceName),
    entryMapLock_(),
    entryMap_(),
    activeThreads_(0),
    noActiveThreadsEvent_(true),
    clientLock_(),
    clientActive_(false),
    random_((int)Common::SequenceNumber::GetNext())
{
    srand(static_cast<unsigned int>(time(NULL)));
}

TestStoreClient::~TestStoreClient()
{
}

void TestStoreClient::Reset()
{
    AcquireExclusiveLock lock1(clientLock_);
    AcquireExclusiveLock lock2(entryMapLock_);
    TestSession::FailTestIf(clientActive_, "Clients should not be active when calling TestStoreClient::Reset for {0}", serviceName_);
    entryMap_.clear();
}

void TestStoreClient::Backup(
    __int64 key, 
    std::wstring const & dir, 
    Store::StoreBackupOption::Enum backupOption, 
    bool postBackupReturn,
    ErrorCodeValue::Enum expectedError)
{
    wstring unusedValue;
    FABRIC_SEQUENCE_NUMBER unusedSequenceNumber;
    ULONGLONG unusedDataLossVersion;

    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().StoreClientTimeout);

    while (!timeout.IsExpired)
    {
        auto entry = GetCurrentEntry(key, true, unusedValue, unusedSequenceNumber, unusedDataLossVersion);
        if(entry)
        {		
            auto const & storeService = entry->StoreService;

            Common::ComPointer<IFabricStorePostBackupHandler> comImpl = 
                Common::make_com<ComTestPostBackupHandler, IFabricStorePostBackupHandler>(postBackupReturn);

            Api::IStorePostBackupHandlerPtr postBackupHandler = Api::WrapperFactory::create_rooted_com_proxy(comImpl.GetRawPointer());

            ManualResetEvent waiter;

            auto operation = storeService->BeginBackup(
                dir, 
                backupOption, 
                postBackupHandler,
                [&](Common::AsyncOperationSPtr const &)
                {
                    waiter.Set();
                },
                nullptr);

            waiter.WaitOne();

            auto error = storeService->EndBackup(operation);

            waiter.Reset();

            TestSession::FailTestIf(
                !error.IsError(expectedError), 
                "Backup: Unexpected error: {0} != {1}", 
                error, 
                expectedError);

            return;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Test Store Service not ready");

            Sleep(RetryWaitMilliSeconds);
        }
    }

    TestSession::FailTest("Timed out waiting for Test Store Service to be ready");
}

void TestStoreClient::Restore(__int64 key, std::wstring const & dir, ErrorCodeValue::Enum expectedError)
{
    wstring unusedValue;
    FABRIC_SEQUENCE_NUMBER unusedSequenceNumber;
    ULONGLONG unusedDataLossVersion;

    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().StoreClientTimeout);

    while (!timeout.IsExpired)
    {
        auto entry = GetCurrentEntry(key, true, unusedValue, unusedSequenceNumber, unusedDataLossVersion);
        if(entry)
        {
            auto const & storeService = entry->StoreService;

            ManualResetEvent waiter;

            auto operation = storeService->BeginRestore(
                dir,
                [&](Common::AsyncOperationSPtr const &)
                {
                    waiter.Set();
                },
                nullptr);

            waiter.WaitOne();

            auto error = storeService->EndRestore(operation);

            TestSession::FailTestIf(
                !error.IsError(expectedError), 
                "Restore: Unexpected error: {0} != {1}", 
                error, 
                expectedError);

            return;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Test Store Service not ready");

            Sleep(RetryWaitMilliSeconds);
        }
    }

    TestSession::FailTest("Timed out waiting for Test Store Service to be ready");
}

void TestStoreClient::CompressionTest(__int64 key, std::vector<std::wstring> const & params, ErrorCodeValue::Enum expectedError)
{
    wstring unusedValue;
    FABRIC_SEQUENCE_NUMBER unusedSequenceNumber;
    ULONGLONG unusedDataLossVersion;

    auto entry = GetCurrentEntry(key, true, unusedValue, unusedSequenceNumber, unusedDataLossVersion);
    if(entry)
    {
        auto const & storeService = entry->StoreService;

        auto error = storeService->CompressionTest(params);

        TestSession::FailTestIf(
            !error.IsError(expectedError), 
            "CompressionTest: Unexpected error: {0} != {1}", 
            error, 
            expectedError);
    }
    else
    {
        TestSession::FailTest("Test Store Service not ready");
    }
}

void TestStoreClient::Put(__int64 key, int keyCount, wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode, bool allowAllError)
{
    for (auto kk=key; kk<key+keyCount; ++kk)
    {
        this->Put(kk, value, useDefaultNamedClient, quorumLost, expectedErrorCode, allowAllError);
    }
}

void TestStoreClient::Put(__int64 key, int keyCount, wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, vector<ErrorCodeValue::Enum> const & expectedErrors, bool allowAllError)
{
    for (auto kk=key; kk<key+keyCount; ++kk)
    {
        this->Put(kk, value, useDefaultNamedClient, quorumLost, expectedErrors, allowAllError);
    }
}

void TestStoreClient::Put(__int64 key, wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, ErrorCode expectedError, bool allowAllError)
{
    vector<ErrorCodeValue::Enum> expectedErrors;
    expectedErrors.push_back(expectedError.ReadValue());

    return Put(key, value, useDefaultNamedClient, quorumLost, expectedErrors, allowAllError);
}

void TestStoreClient::Put(__int64 key, wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, vector<ErrorCodeValue::Enum> const & expectedErrors, bool allowAllError)
{
    quorumLost = false;

    TestSession::WriteNoise(TraceSource, "Put at service {0} for key {1}", serviceName_, key);

    ErrorCode error;
    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().StoreClientTimeout);
    bool done = false;
    bool matchedExpectedError = false;

    do
    {
        wstring currentValue;
        FABRIC_SEQUENCE_NUMBER currentVersion;
        ULONGLONG currentDataLossVersion;

        FABRIC_SEQUENCE_NUMBER newVersion = 0;
        ULONGLONG dataLossVersion = 0;
        wstring partitionId;

        // Don't hold onto store service reference while retrying resolution
        //
        {
            auto entry = GetCurrentEntry(key, useDefaultNamedClient, currentValue, currentVersion, currentDataLossVersion);

            if (entry)
            {
                auto const & storeService = entry->StoreService;

                partitionId = storeService->GetPartitionId();

                error = storeService->Put(key, value, currentVersion, serviceName_, newVersion, dataLossVersion);
            }
            else
            {
                error = ErrorCodeValue::FabricTestServiceNotOpen;
            }
        }

        bool updated = true;

        if (error.IsSuccess())
        {
            UpdateEntry(key, value, newVersion, dataLossVersion);
        }
        else if (error.IsError(ErrorCodeValue::FabricTestVersionDoesNotMatch))
        {
            TestSession::WriteNoise(TraceSource, "Put for service {0} at key {1} resulted in error FabricTestVersionDoesNotMatch", serviceName_, key);
            TestSession::FailTestIf(newVersion == currentVersion, "Versions cannot match because Put returned FabricTestVersionDoesNotMatch");
            TestSession::FailTestIf(partitionId.empty(), "Empty partitionId for service {0} while putting key {1}", serviceName_, key);

            if (newVersion < currentVersion)
            {
                CheckForDataLoss(key, partitionId, currentDataLossVersion, dataLossVersion); 
            }

            //If newVersion is > then currentVersion that means the entry we have is out of date and 
            //some other thread updated value before us or that although replication failed (possibly due to a node closing)
            //the new primary did receive the operation
            UpdateEntry(key);
        }
        else
        {
            updated = false;
        }

        auto findIt = find_if(expectedErrors.begin(), expectedErrors.end(), [&error](ErrorCodeValue::Enum item) { return error.IsError(item); });
        matchedExpectedError = (findIt != expectedErrors.end());
        
        if (!updated && !matchedExpectedError)
        {
            TestSession::WriteNoise(TraceSource, "Put for service {0} at key {1} resulted in error {2}", serviceName_, key, error);

            Guid fuId = GetFUId(key);
            if(FABRICSESSION.FabricDispatcher.IsQuorumLostFU(fuId) || FABRICSESSION.FabricDispatcher.IsRebuildLostFU(fuId))
            {
                TestSession::WriteNoise(TraceSource, "Quorum or Rebuild lost at {0}:{1} for key {2} so skipping operation", serviceName_, fuId, key);
                Sleep(RetryWaitMilliSeconds);
                quorumLost = true;
                return;
            }
            else
            {
                //We might have out of date pointer to StoreService. Need to resolve again
                ResolveService(key, useDefaultNamedClient, allowAllError);
            }
        }

        done = (matchedExpectedError || timeout.IsExpired);

        if(!done)
        {
            //Sleep before retrying.
            Sleep(RetryWaitMilliSeconds);
        }

    } while(!done);

    if (matchedExpectedError)
    {
        TestSession::WriteNoise(TraceSource, "Put at service {0} for key {1} succeeded with expectedErrors {2}", serviceName_, key, expectedErrors);
    }
    else
    {
        TestSession::FailTest("Failed to write to key {0} at service {1} because of error {2}, expectedErrors {3}", key, serviceName_, error, expectedErrors);
    }
}

void TestStoreClient::Get(__int64 key, int keyCount, bool useDefaultNamedClient, __out vector<wstring> & values, __out vector<bool> & exists, __out bool & quorumLost)
{
    values.clear();
    exists.clear();

    values.resize(keyCount);
    exists.resize(keyCount);

    for (auto kk=key; kk<key+keyCount; ++kk)
    {
        wstring value;
        bool keyExists;

        this->Get(kk, useDefaultNamedClient, value, keyExists, quorumLost);

        values[kk-key] = value;
        exists[kk-key] = keyExists;
    }
}

void TestStoreClient::Get(__int64 key, bool useDefaultNamedClient, __out wstring & value, __out bool & keyExists, __out bool & quorumLost)
{
    quorumLost = false;
    keyExists = false;
    TestSession::WriteNoise(TraceSource, "Get at service {0} for key {1}", serviceName_, key);
    //TODO: Make timeout value configurable if required
    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().StoreClientTimeout);
    bool isSuccess = false;
    bool done = false;
    do
    {
        wstring currentValue;
        FABRIC_SEQUENCE_NUMBER currentVersion;
        ULONGLONG currentDataLossVersion;

        FABRIC_SEQUENCE_NUMBER version = 0;
        ULONGLONG dataLossVersion = 0;
        ErrorCode error;
        wstring partitionId;

        {
            auto entry = GetCurrentEntry(key, useDefaultNamedClient, currentValue, currentVersion, currentDataLossVersion);
            if(entry)
            {
                auto const & storeService = entry->StoreService;

                partitionId = storeService->GetPartitionId();

                error = storeService->Get(key, serviceName_, value, version, dataLossVersion); 
            }
            else
            {
                error = ErrorCode(ErrorCodeValue::FabricTestServiceNotOpen);
            }
        }

        bool resolve = false;

        if(error.IsSuccess())
        {
            //Only do value validation if there hasn't been any data loss AND no other new version for the given key is inserted into the map.
            if(dataLossVersion == currentDataLossVersion && version == currentVersion)
            {
                TestSession::FailTestIf(value.compare(currentValue) != 0, "Value in store at service {0} for key {1} does not match expected {2}, actual {3}",
                    serviceName_,
                    key,
                    currentValue.length(),
                    currentValue,
                    value.length(),
                    value);

                isSuccess = true;
            }
            else if(version < currentVersion)
            {
                CheckForDataLoss(key, partitionId, currentDataLossVersion, dataLossVersion);
            }

            //If version is > then currentVersion that means the entry we have is out of date and 
            //some other thread updated value before us or that although replication failed (possibly due to a node closing)
            //the new primary did receive the operation
            UpdateEntry(key);
        }
        else if(error.ReadValue() == ErrorCodeValue::FabricTestKeyDoesNotExist)
        {
            TestSession::WriteNoise(TraceSource, "Get for service {0} at key {1} resulted in error {2}", serviceName_, key, error.ReadValue());
            bool isInitialEntry = (currentVersion == InitialVersion && currentValue.compare(InitialValue) == 0);
            if(!isInitialEntry)
            {
                CheckForDataLoss(key, partitionId, currentDataLossVersion, dataLossVersion);
            }

            keyExists = false;
            return;
        }
        else
        {
            resolve = true;
        }
        
        if (resolve)
        {
            TestSession::WriteNoise(TraceSource, "Get for service {0} at key {1} resulted in error {2}", serviceName_, key, error.ReadValue());
            Guid fuId = GetFUId(key);
            if(FABRICSESSION.FabricDispatcher.IsQuorumLostFU(fuId)  || FABRICSESSION.FabricDispatcher.IsRebuildLostFU(fuId))
            {
                TestSession::WriteNoise(TraceSource, "Quorum or Rebuild lost at {0}:{1} for key {2} so skipping operation", serviceName_, fuId, key);
                Sleep(RetryWaitMilliSeconds);
                quorumLost = true;
                return;
            }
            else
            {
                //We might have out of date pointer to StoreService. Need to resolve again
                ResolveService(key, useDefaultNamedClient);
            }
        }

        done = (isSuccess || timeout.IsExpired);
        if(!done)
        {
            //Sleep before retrying.
            Sleep(RetryWaitMilliSeconds);
        }

    } while(!done);

    if(!isSuccess)
    {
        TestSession::FailTest("Failed to read key {0} at service {1}", key, serviceName_);
    }

    keyExists = true;
    TestSession::WriteNoise(TraceSource, "Get at service {0} for key {1} succeeded", serviceName_, key);
}

void TestStoreClient::GetAll(__out vector<pair<__int64, wstring>> & kvpairs, __out bool & quorumLost)
{
    quorumLost = false;
    TestSession::WriteNoise(TraceSource, "GetAll at service {0}", serviceName_);

    ErrorCode error;
    vector<ITestStoreServiceSPtr> storeServices = GetAllEntries();
    for (auto & storeService : storeServices)
    {
        // GetAll should append whatever key value pairs it found to the specified 'kvpairs'
        error = storeService->GetAll(serviceName_, kvpairs);
        if (!error.IsSuccess())
        {
            TestSession::FailTest("Failed to getAll at service {0}", serviceName_);
        }
    }

    TestSession::WriteNoise(TraceSource, "GetAll at service {0} succeeded", serviceName_);
}

void TestStoreClient::GetKeys(__int64 first, __int64 last, __out vector<__int64> & keys, __out bool & quorumLost)
{
    quorumLost = false;
    TestSession::WriteNoise(TraceSource, "GetKeys at service {0}", serviceName_);

    ErrorCode error;
    vector<ITestStoreServiceSPtr> storeServices = GetAllEntries();
    for (auto & storeService : storeServices)
    {
        // GetRange should append whatever key value pairs it found to the specified 'kvpairs'
        error = storeService->GetKeys(serviceName_, first, last, keys);
        if (!error.IsSuccess())
        {
            TestSession::FailTest("Failed to getKeys at service {0}", serviceName_);
        }
    }

    TestSession::WriteNoise(TraceSource, "GetKeys at service {0} succeeded", serviceName_);
}

void TestStoreClient::Delete(__int64 key, int keyCount, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode, bool allowAllError)
{
    for (auto kk=key; kk<key+keyCount; ++kk)
    {
        this->Delete(kk, useDefaultNamedClient, quorumLost, expectedErrorCode, allowAllError);
    }
}

void TestStoreClient::Delete(__int64 key, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode, bool allowAllError)
{
    quorumLost = false;
    TestSession::WriteNoise(TraceSource, "Delete at service {0} for key {1}", serviceName_, key);
    //TODO: Make timeout value configurable if required
    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().StoreClientTimeout);
    ErrorCode error;
    bool done = false;
    do
    {
        wstring currentValue;
        FABRIC_SEQUENCE_NUMBER currentVersion;
        ULONGLONG currentDataLossVersion;
        FABRIC_SEQUENCE_NUMBER newVersion = 0;
        ULONGLONG dataLossVersion = 0;
        wstring partitionId;

        {
            auto entry = GetCurrentEntry(key, useDefaultNamedClient, currentValue, currentVersion, currentDataLossVersion);
            if(entry)
            {
                auto const & storeService = entry->StoreService;

                partitionId = storeService->GetPartitionId();

                error = storeService->Delete(key, currentVersion, serviceName_, newVersion, dataLossVersion);
            }
            else
            {
                error = ErrorCode(ErrorCodeValue::FabricTestServiceNotOpen);
            }
        }

        bool resolve = false;

        if(error.IsSuccess())
        {
            DeleteEntry(key);
        }
        else if(error.ReadValue() == ErrorCodeValue::FabricTestVersionDoesNotMatch)
        {
            TestSession::WriteNoise(TraceSource, "Delete for service {0} at key {1} resulted in error FabricTestVersionDoesNotMatch", serviceName_, key);
            TestSession::FailTestIf(newVersion == currentVersion, "Versions cannot match because Delete returned FabricTestVersionDoesNotMatch");
            if(newVersion < currentVersion)
            {
                CheckForDataLoss(key, partitionId, currentDataLossVersion, dataLossVersion); 
            }

            //If newVersion is > then currentVersion that means the entry we have is out of date and 
            //some other thread updated value before us or that although replication failed (possibly due to a node closing)
            //the new primary did receive the operation
            UpdateEntry(key);
        }
        else if (error.ReadValue() != expectedErrorCode.ReadValue())
        {
            resolve = true;
        }
        
        if (resolve)
        {
            TestSession::WriteNoise(TraceSource, "Delete for service {0} at key {1} resulted in error {2}", serviceName_, key, error.ReadValue());
            Guid fuId = GetFUId(key);
            if(FABRICSESSION.FabricDispatcher.IsQuorumLostFU(fuId) || FABRICSESSION.FabricDispatcher.IsRebuildLostFU(fuId))
            {
                TestSession::WriteNoise(TraceSource, "Quorum or Rebuild lost at {0}:{1} for key {2} so skipping operation", serviceName_, fuId, key);
                Sleep(RetryWaitMilliSeconds);
                quorumLost = true;
                return;
            }
            else
            {
                //We might have out of date pointer to StoreService. Need to resolve again
                ResolveService(key, useDefaultNamedClient, allowAllError);
            }
        }

        done = (error.ReadValue() == expectedErrorCode.ReadValue() || timeout.IsExpired);
        if(!done)
        {
            //Sleep before retrying.
            Sleep(RetryWaitMilliSeconds);
        }

    } while(!done);

    if(error.ReadValue() != expectedErrorCode.ReadValue())
    {
        TestSession::FailTest("Failed to write to key {0} at service {1} because of error {2}, expectedErrorCode {3}", key, serviceName_, error.ReadValue(), expectedErrorCode.ReadValue());
    }

    TestSession::WriteNoise(TraceSource, "Delete at service {0} for key {1} succeeded with expectedError {2}", serviceName_, key, expectedErrorCode.ReadValue());
}

void TestStoreClient::StartClient()
{
    AcquireExclusiveLock lock(clientLock_);
    clientActive_ = true;
}

void TestStoreClient::WaitForActiveThreadsToFinish()
{
    {
        AcquireExclusiveLock lock(clientLock_);
        clientActive_ = false;
    }

    //TODO: Make timeout value confiurable if required
    bool result = noActiveThreadsEvent_.WaitOne(TimeSpan::FromSeconds(900));
    TestSession::FailTestIfNot(result, "Clients did not finish in time");
    TestSession::WriteInfo(TraceSource, "All Client threads for {0} done", serviceName_);
}

void TestStoreClient::DoWork(ULONG threadId, __int64 key, bool doPut)
{
    {
        AcquireExclusiveLock lock(clientLock_);
        if(!clientActive_)
        {
            return;
        }

        if(activeThreads_ == 0)
        {
            noActiveThreadsEvent_.Reset();
        }

        activeThreads_++;
    }

    TestSession::WriteNoise(TraceSource, "Client thread {0} for {1} invoked", threadId, serviceName_);

    bool useDefaultNamedClient = true;
    if(!EntryExists(key) || doPut)
    {
        bool quorumLost;
        wstring data;

        if (FabricTestSessionConfig::GetConfig().MinClientPutDataSizeInBytes >= 0)
        {    
            GenerateRandomClientPutData(data);
        }
        else
        {
            data = Guid::NewGuid().ToString();
        }

        Put(key, data, useDefaultNamedClient, quorumLost);
    }
    else
    {
        wstring value;
        bool keyExists;
        bool quorumLost;

        Get(key, useDefaultNamedClient, value, keyExists, quorumLost);
    }

    {
        AcquireExclusiveLock lock(clientLock_);
        activeThreads_--;
        TestSession::FailTestIf(activeThreads_ < 0, "ActiveThreads can never be less than zero");
        if(activeThreads_ == 0)
        {
            noActiveThreadsEvent_.Set();
        }
    }
}

void TestStoreClient::GenerateRandomClientPutData(__out wstring & data)
{
    int minDataSize = FabricTestSessionConfig::GetConfig().MinClientPutDataSizeInBytes;
    int maxDataSize = FabricTestSessionConfig::GetConfig().MaxClientPutDataSizeInBytes;

    int dataSize = random_.Next(minDataSize, maxDataSize);

    data.reserve(dataSize);
    
    for (int i = 0; i < dataSize; ++i)
    {
        // ASCII alphanumeric chars between 32 and 126
        data.push_back((char)((rand() % 94) + 32));
    }
}

bool TestStoreClient::EntryExists(__int64 key)
{
    AcquireExclusiveLock lock(entryMapLock_);
    auto iterator = entryMap_.find(key);
    if(iterator == entryMap_.end())
    {
        return false;
    }

    if(entryMap_[key]->Version == InitialVersion && entryMap_[key]->Value.compare(InitialValue) == 0)
    {
        return false;
    }

    return true;
}

shared_ptr<TestStoreClient::StoreServiceEntry> TestStoreClient::GetCurrentEntry(__int64 key, bool useDefaultNamedClient, __out wstring & value, __out FABRIC_SEQUENCE_NUMBER & version, __out ULONGLONG & dataLossVersion)
{
    bool newEntryOrEmptyStoreServiceWPtr = false;
    {
        AcquireExclusiveLock lock(entryMapLock_);
        auto iterator = entryMap_.find(key);
        newEntryOrEmptyStoreServiceWPtr = (iterator == entryMap_.end()) || (!entryMap_[key]->StoreService.lock());
    }

    ITestStoreServiceWPtr storeServiceWPtr;
    //Blocking call to resolve should not be under a lock
    Guid fuId;
    if(newEntryOrEmptyStoreServiceWPtr)
    {
        fuId = FABRICSESSION.FabricDispatcher.ServiceMap.ResolveFuidForKey(serviceName_, key);
        ITestStoreServiceSPtr storeServiceSPtr = FABRICSESSION.FabricDispatcher.ResolveService(serviceName_, key, fuId, useDefaultNamedClient);
        if(storeServiceSPtr)
        {
            storeServiceWPtr = weak_ptr<ITestStoreService>(storeServiceSPtr);
        }
    }

    AcquireExclusiveLock lock(entryMapLock_);
    auto iterator = entryMap_.find(key);
    if(iterator == entryMap_.end())
    {
        entryMap_[key] = make_shared<TestClientEntry>(fuId, InitialValue, InitialVersion, move(storeServiceWPtr), InitialDataLossVersion);
    }
    else if (storeServiceWPtr.lock())
    {
        // We resolved a new weak pointer so it should be set
        entryMap_[key]->StoreService = move(storeServiceWPtr);
    }

    value = entryMap_[key]->Value;
    version = entryMap_[key]->Version;
    dataLossVersion = entryMap_[key]->DataLossVersion;
    auto storeServiceSPtr = entryMap_[key]->StoreService.lock();

    return (storeServiceSPtr ? make_shared<StoreServiceEntry>(move(storeServiceSPtr)) : nullptr);
}

vector<ITestStoreServiceSPtr> TestStoreClient::GetAllEntries()
{
    vector<ITestStoreServiceSPtr> result;
    set<Guid> guids;

    AcquireExclusiveLock lock(entryMapLock_);
    for (auto & entry : entryMap_)
    {
        ITestStoreServiceSPtr temp = entry.second->StoreService.lock();
        if (!temp || guids.find(entry.second->Id) != guids.end())
        {
            continue;
        }
        result.push_back(temp);
        guids.insert(entry.second->Id);
    }
    
    return result;
}

Guid TestStoreClient::GetFUId(__int64 key)
{
    AcquireExclusiveLock lock(entryMapLock_);
    return entryMap_[key]->Id;
}

void TestStoreClient::ResolveService(__int64 key, bool useDefaultNamedClient, bool allowAllError)
{
    ITestStoreServiceWPtr storeServiceWPtr = weak_ptr<ITestStoreService>(FABRICSESSION.FabricDispatcher.ResolveService(serviceName_, key, GetFUId(key), useDefaultNamedClient, allowAllError));
    AcquireExclusiveLock lock(entryMapLock_);
    entryMap_[key]->StoreService = move(storeServiceWPtr);
}

void TestStoreClient::DeleteEntry(__int64 key)
{
    ITestStoreServiceSPtr storeServiceSPtr;
    {
        AcquireExclusiveLock lock(entryMapLock_);
        storeServiceSPtr = entryMap_[key]->StoreService.lock();
    }

    if(storeServiceSPtr)
    {
        AcquireExclusiveLock lock(entryMapLock_);

        entryMap_.erase(key);
    }
}

void TestStoreClient::UpdateEntry(__int64 key)
{
    FABRIC_SEQUENCE_NUMBER version;
    wstring value;
    ULONGLONG dataLossVersion;
    ITestStoreServiceSPtr storeServiceSPtr;
    {
        AcquireExclusiveLock lock(entryMapLock_);
        storeServiceSPtr = entryMap_[key]->StoreService.lock();
    }

    if(storeServiceSPtr)
    {
        ErrorCode error = storeServiceSPtr->Get(key, serviceName_, value, version, dataLossVersion); 

        AcquireExclusiveLock lock(entryMapLock_);
        if(error.IsSuccess())
        {
            entryMap_[key]->Value = value;
            entryMap_[key]->Version = version;
            entryMap_[key]->DataLossVersion = dataLossVersion;
        }
    }
}

void TestStoreClient::UpdateEntry(__int64 key, wstring const& value, FABRIC_SEQUENCE_NUMBER version, ULONGLONG dataLossVersion)
{
    AcquireExclusiveLock lock(entryMapLock_);

    if(version == entryMap_[key]->Version && dataLossVersion == entryMap_[key]->DataLossVersion)
    {
        TestSession::FailTestIf(value.compare(entryMap_[key]->Value) != 0, "Value in store at service {0} for key {1} does not match expected {2}, actual {3}",
            serviceName_,
            key,
            entryMap_[key]->Value,
            value);
    }
    else if(version >= entryMap_[key]->Version)
    {
        entryMap_[key]->Value = value;
        entryMap_[key]->Version = version;
        entryMap_[key]->DataLossVersion = dataLossVersion;
    }
    else if(version < entryMap_[key]->Version)
    {
        //There is a more up to date version in our map
        //Dont need to update
    }
}

void TestStoreClient::CheckForDataLoss(__int64 key, wstring const& partitionId, ULONGLONG clientDataLossVersion, ULONGLONG serviceDataLossVersion)
{
    TestSession::WriteNoise(TraceSource, "CheckForDataLoss called for {0}:{1} for key {2}", serviceName_, partitionId, key);
    //serviceDataLossVersion == clientDataLossVersion that means data loss is not expected and we should fail fast here
    TestSession::FailTestIf(serviceDataLossVersion == clientDataLossVersion, 
        "Data loss: Service {0}, key {1} and partition {2}. ClientDataLossVersion {3}, serviceDataLossVersion {4}", 
        serviceName_, key, partitionId, clientDataLossVersion, serviceDataLossVersion);

    //serviceDataLossVersion < clientDataLossVersion: This should not happen so fail.
    TestSession::FailTestIf(serviceDataLossVersion < clientDataLossVersion, "Clients DataLossVersion {0} is greater than the version in FailoverUnit {1}", clientDataLossVersion, serviceDataLossVersion);

    //Data loss is expected so we updat our entry with the current one at the service
    if(entryMap_[key]->DataLossVersion < serviceDataLossVersion)
    {
        FABRIC_SEQUENCE_NUMBER version;
        wstring value;
        ULONGLONG dataLossVersion;
        ITestStoreServiceSPtr storeServiceSPtr;
        {
            AcquireExclusiveLock lock(entryMapLock_);
            storeServiceSPtr = entryMap_[key]->StoreService.lock();
        }

        if(storeServiceSPtr)
        {
            ErrorCode error = storeServiceSPtr->Get(key, serviceName_, value, version, dataLossVersion); 

            AcquireExclusiveLock lock(entryMapLock_);
            if(error.IsSuccess())
            {
                entryMap_[key]->Value = value;
                entryMap_[key]->Version = version;
                entryMap_[key]->DataLossVersion = dataLossVersion;
            }
            else if(error.ReadValue() == ErrorCodeValue::FabricTestKeyDoesNotExist)
            {
                entryMap_[key]->Value = InitialValue;
                entryMap_[key]->Version = InitialVersion;
                entryMap_[key]->DataLossVersion = dataLossVersion;
            }
            else
            {
                TestSession::WriteInfo(TraceSource, "Get for service {0}, key {1} resulted in error {2} inside of CheckForDataLoss", serviceName_, key, error.ReadValue());
            }
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Service pointer null for service {0}, key {1} inside of CheckForDataLoss", serviceName_, key);
        }
    }
}
