//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <vector>

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel::ModelV2;
    using namespace ServiceModel;

    StringLiteral const TestSource("ModelV2Test");
    std::wstring const TestFilesDirectory(L"modelV2TestFiles");
    std::wstring const ValidFileSuffix(L"_valid");

    class Application
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
    public:
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"application", applicationDescription)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(applicationDescription);

        ApplicationDescription applicationDescription;
    };

    class ModelV2Test
    {
    public:

        wstring GetTestFilesDirectory()
        {
            auto folder = Path::Combine(Directory::GetCurrentDirectory(), TestFilesDirectory);

            if (!Directory::Exists(folder))
            {
                wstring unitTestsFolder;
#if !defined(PLATFORM_UNIX)
                VERIFY_IS_TRUE(
                    Environment::ExpandEnvironmentStringsW(L"%_NTTREE%\\FabricUnitTests", unitTestsFolder),
                    L"ExpandEnvironmentStringsW");
#else
                VERIFY_IS_TRUE(
                    Environment::ExpandEnvironmentStringsW(L"%_NTTREE%/FabricUnitTests", unitTestsFolder),
                    L"ExpandEnvironmentStringsW");
#endif
                return Path::Combine(unitTestsFolder, TestFilesDirectory);
            }
            else
            {
                return folder;
            }
        }

        void ReadFileContents(std::wstring const &fileName, ByteBufferUPtr &bytes)
        {
            File testFile;
            auto error = testFile.TryOpen(fileName, FileMode::Open, FileAccess::Read, FileShare::None, FileAttributes::Normal);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "TryOpen {0} failed: {1}", fileName, error);

            int64 size = 0;
            error = testFile.GetSize(size);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "GetSize {0} failed: {1}", fileName, error);

            bytes = make_unique<ByteBuffer>(size);

            DWORD bytesRead;
            error = testFile.TryRead2(bytes->data(), static_cast<int>(size), bytesRead);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "TryRead2 failed: {0}", error);
        }

    };

    BOOST_FIXTURE_TEST_SUITE(ModelV2TestSuite,ModelV2Test)

    BOOST_AUTO_TEST_CASE(ApplicationDescriptionTest)
    {
        auto testFilesFolder = GetTestFilesDirectory();
        VERIFY_IS_TRUE_FMT(Directory::Exists(testFilesFolder), "Test files directory {0} does not exist", testFilesFolder);

        auto testFiles = Directory::GetFiles(testFilesFolder);
        for (auto &fileName : testFiles)
        {
            ByteBufferUPtr bytes;
            ReadFileContents(Path::Combine(testFilesFolder, fileName), bytes);

            Application application;

            auto error = JsonHelper::Deserialize(application, bytes);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = application.applicationDescription.TryValidate(fileName);           
            if (StringUtility::ContainsCaseInsensitive<wstring>(fileName, L"invalid"))
            {
                VERIFY_IS_FALSE_FMT(error.IsSuccess(), error.Message);
            }
            else
            {
                VERIFY_IS_TRUE_FMT(error.IsSuccess(), error.Message);
            }

            // Serialize and then deserialize to stream
            vector<byte> buffer;
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&application, buffer).IsSuccess());

            Application deserialized;
            auto error2 = FabricSerializer::Serialize(&application, buffer);
            VERIFY_IS_TRUE(error2.IsSuccess());
            error2 = FabricSerializer::Deserialize(deserialized, buffer);
            VERIFY_IS_TRUE(error2.IsSuccess());
        }
    }

    BOOST_AUTO_TEST_CASE(ResourceNameValidationTest)
    {
        ResourceDescription description(*Constants::ModelV2ReservedDnsNameCharacters);

        description.Name = L"myService";
        auto error = description.TryValidate(L"");
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), description.Name);

        description.Name = L"myService.myApp";
        error = description.TryValidate(L"");
        VERIFY_IS_FALSE_FMT(error.IsSuccess(), description.Name);
    }

#if !defined(PLATFORM_UNIX)

    BOOST_AUTO_TEST_CASE(VolumeRefTest)
    {
        IndependentVolumeRef volRef(L"myVol", L"c:\\");
        VERIFY_IS_TRUE(volRef.TryValidate(L"").IsSuccess())
    
        volRef.Test_SetDestinationPath(L"c:");
        VERIFY_IS_TRUE(volRef.TryValidate(L"").IsSuccess())

        volRef.Test_SetDestinationPath(L"c:\\path");
        VERIFY_IS_TRUE(volRef.TryValidate(L"").IsSuccess())

        volRef.Test_SetDestinationPath(L"c");
        VERIFY_IS_FALSE(volRef.TryValidate(L"").IsSuccess())

        volRef.Test_SetDestinationPath(L"/data");        
        VERIFY_IS_FALSE(volRef.TryValidate(L"").IsSuccess())
    }

#endif

    BOOST_AUTO_TEST_SUITE_END()
}

