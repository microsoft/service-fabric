// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class TestStoreClient : public std::enable_shared_from_this<TestStoreClient>
    {
        DENY_COPY(TestStoreClient);

    public:
        TestStoreClient(std::wstring const & serviceName);
        virtual ~TestStoreClient();

        void Put(__int64 key, int keyCount, std::wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode = Common::ErrorCodeValue::Success, bool allowAllError = false);
        void Put(__int64 key, int keyCount, std::wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, std::vector<Common::ErrorCodeValue::Enum> const & expectedErrors, bool allowAllError = false);

        void Get(__int64 key, int keyCount, bool useDefaultNamedClient, __out std::vector<std::wstring> & values, __out std::vector<bool> & exists, __out bool & quorumLost);
        void GetAll(__out std::vector<std::pair<__int64,std::wstring>> & kvpairs, __out bool & quorumLost);
        void GetKeys(__int64 first, __int64 last, __out std::vector<__int64> & keys, __out bool & quorumLost);
        void Delete(__int64 key, int keyCount, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode = Common::ErrorCodeValue::Success, bool allowAllError = false);

        void DoWork(ULONG threadId, __int64 key, bool doPut);
        void StartClient();
        void WaitForActiveThreadsToFinish();
        void Reset();

        void Backup(__int64 key, std::wstring const & dir, StoreBackupOption::Enum backupOption, bool postBackupReturn, Common::ErrorCodeValue::Enum expectedError);
        void Restore(__int64 key, std::wstring const & dir, Common::ErrorCodeValue::Enum expectedError);
        void CompressionTest(__int64 key, std::vector<std::wstring> const &, Common::ErrorCodeValue::Enum expectedError);

    private:
        class TestClientEntry;
        class StoreServiceEntry;

        void Put(__int64 key, std::wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode = Common::ErrorCodeValue::Success, bool allowAllError = false);
        void Put(__int64 key, std::wstring const & value, bool useDefaultNamedClient, __out bool & quorumLost, std::vector<Common::ErrorCodeValue::Enum> const & expectedErrors, bool allowAllError = false);
        void Get(__int64 key, bool useDefaultNamedClient, __out std::wstring & value, __out bool & keyExists, __out bool & quorumLost);
        void Delete(__int64 key, bool useDefaultNamedClient, __out bool & quorumLost, Common::ErrorCode expectedErrorCode = Common::ErrorCodeValue::Success, bool allowAllError = false);

        void TestStoreClient::GenerateRandomClientPutData(__out std::wstring & data);

        std::wstring serviceName_;
        Common::ExclusiveLock entryMapLock_;
        std::map<__int64, std::shared_ptr<TestClientEntry>> entryMap_;
        LONG activeThreads_;
        Common::ManualResetEvent noActiveThreadsEvent_;
        Common::ExclusiveLock clientLock_;
        bool clientActive_;
        Common::Random random_;

        static std::wstring const InitialValue;
        static FABRIC_SEQUENCE_NUMBER const InitialVersion;
        static ULONGLONG const InitialDataLossVersion;
        static int const RetryWaitMilliSeconds;

        void CheckForDataLoss(__int64 key, std::wstring const& partitionId, ULONGLONG clientDataLossVersion, ULONGLONG serviceDataLossVersion);
        std::shared_ptr<StoreServiceEntry> GetCurrentEntry(__int64 key, bool useDefaultNamedClient, __out std::wstring & value, __out FABRIC_SEQUENCE_NUMBER & version, __out ULONGLONG & dataLossVersion);
        std::vector<ITestStoreServiceSPtr> GetAllEntries();
        Common::Guid GetFUId(__int64 key);
        void DeleteEntry(__int64 key);
        void UpdateEntry(__int64 key);
        bool EntryExists(__int64 key);
        void UpdateEntry(__int64 key, std::wstring const& value, FABRIC_SEQUENCE_NUMBER version, ULONGLONG dataLossVersion);
        void ResolveService(__int64 key, bool useDefaultNamedClient, bool allowAllError = false);
    };

    typedef std::shared_ptr<TestStoreClient> TestStoreClientSPtr;
}
