// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;
    class NativeImageStoreExecutor
    {
        DENY_COPY(NativeImageStoreExecutor);

    public:
        static int const MinSmallFileSizeInBytes = 1024;
        static int const MaxSmallFileSizeInBytes = 1024 * MinSmallFileSizeInBytes;

        static int const MinLargeFileSizeInBytes = 1024 * 1024;
        static int const MaxLargeFileSizeInBytes = 512 * MinLargeFileSizeInBytes;

        static Common::GlobalWString CreateDirectoryCommand;
        static Common::GlobalWString CreateFileCommand;
        static Common::GlobalWString UploadCommand;
        static Common::GlobalWString DownloadCommand;
        static Common::GlobalWString DeleteCommand;
        static Common::GlobalWString ListCommand;
        static Common::GlobalWString WaitCommand;
        static Common::GlobalWString VerifyCommand;

        NativeImageStoreExecutor(FabricTestDispatcher & dispatcher);
        ~NativeImageStoreExecutor();

        bool ExecuteCommand(std::wstring const & command, Common::StringCollection const & params);

        static bool IsNativeImageStoreClientCommand(std::wstring const & command);

    private:
        struct ResourceInfo;
        typedef std::shared_ptr<ResourceInfo> ResourceInfoSPtr;
        typedef std::map<std::wstring, ResourceInfoSPtr> CreatedResourceMap;
        typedef std::map<std::wstring, ResourceInfoSPtr> UploadedResourceMap;

        bool Upload(Common::StringCollection const & params);
        bool Download(Common::StringCollection const & params);
        bool Delete(Common::StringCollection const & params);
        bool Wait();
        bool Verify();
        bool ValidateStore(
            std::wstring const & storeShareLocation,
            std::vector<Management::FileStoreService::StoreFileInfo> const & metadataList,
            UploadedResourceMap const & uploadedResources);
        bool ValidateFile(
            std::wstring const & storeRelativePath, 
            std::wstring const & localFilePath, 
            std::wstring const & storeShareLocation, 
            std::vector<Management::FileStoreService::StoreFileInfo> const & metadataList);         

        void UploadToService(std::wstring const & resouceName, std::wstring const & storeRelativePath, bool const shouldOverwrite, Common::ErrorCode const & expectedError);
        void DownloadFromService(std::wstring const & storeRelativePath, Common::ErrorCode const & expectedError);
        void DeleteFromService(std::wstring const & storeRelativePath, Common::ErrorCode const & expectedError);

        bool CreateTestFile(Common::StringCollection const & params);
        bool CreateTestDirectory(Common::StringCollection const & params);
        bool GenerateFile(std::wstring const & fileName, bool shouldGenerateLargeFile);

        void PostOperation(std::function<void()> const & operation);

        static bool IsContentEqual(std::wstring const & path1, std::wstring const & path2);
        static bool IsFileContentEqual(std::wstring const & path1, std::wstring const & path2);
        static Common::ErrorCode TryReadFile(std::wstring const & fileName, __out vector<byte> & fileContent);

        enum Size
        {
            Small,
            Large,
            Mixed
        };

        Size FromString(std::wstring const & str)
        {
            if (Common::StringUtility::AreEqualCaseInsensitive(str, L"Small"))
            {
                return Size::Small;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(str, L"Large"))
            {
                return Size::Large;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(str, L"Mixed"))
            {
                return Size::Mixed;
            }
            else
            {
                Assert::CodingError("Unknown string: {0}", str);
            }
        }

        bool ShouldGenerateLargeFile(Size const & val)
        {
            Random rand;
            bool shouldGenerateLargeFile;
            if(val == Size::Mixed)
            {
                shouldGenerateLargeFile = ((rand.Next(1, 100) % 2) == 1);
            }
            else
            {
                shouldGenerateLargeFile = (val == Size::Large);
            }

            return shouldGenerateLargeFile;
        }

    private:
        FabricTestDispatcher & dispatcher_;
        UploadedResourceMap uploadedResourceMap_;
        CreatedResourceMap createdResourcesMap_;
        Common::RwLock lock_;
        Common::Random rand_;
        Common::atomic_uint64 pendingAsyncOperationCount_;
        Common::ManualResetEvent pendingOperationsEvent_;
    };
}
