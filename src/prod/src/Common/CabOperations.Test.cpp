// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"



#include <boost/test/unit_test.hpp> 
#include "Common/boost-taef.h"
#include "Common/CabOperations.h"
#include "Common/ProcessUtility.h"

using namespace std;

std::wstring PackageBaseDir = L"WindowsFabricPackage";

std::wstring BinFileNamesArray[] = {
    L"bin\\FabricHost.exe",
    L"bin\\Fabric\\DCA.Code\\AzureFileUploader.dll",
    L"bin\\Fabric\\DCA.Code\\System.Spatial.dll",
    L"bin\\Fabric\\Fabric.Code\\Cfx.Assert.dll",
    L"bin\\Fabric\\Fabric.Code\\en-us\\FabricResources.dll.mui",
    L"bin\\Fabric\\Fabric.Code\\en-us\\KtlEvents.dll.mui",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\FileStoreService.Manifest.Current.xml",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\FileStoreService.Code.Current\\FileStoreService.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\IS.Code.Current\\FabricIS.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\NM.Code.Current\\NamespaceManager.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\RM.Code.Current\\FabricRM.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\FAS.Code.Current\\FabricFaultAnalysisService.dll",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\FAS.Code.Current\\FabricFAS.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\TVS.Code.Current\\TokenValidationSvc.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\US.Code.Current\\FabricUS.exe",
    L"bin\\Fabric\\Fabric.Code\\__FabricSystem_App4294967295\\GRM.Code.Current\\FabricGRM.exe",
    L"bin\\Fabric\\Fabric.Data\\html\\ServiceFabric.html",
    L"bin\\ServiceFabric\\Microsoft.ServiceFabric.Powershell.dll",
    L"EULA\\EULA_ENU.rtf",
};
std::wstring USFileName = L"FabricInstallerService.Code\\FabricInstallerService.exe";
std::wstring UpdateFileName = L"update.mum";
std::wstring TestBaseDirectory = L"CabOperationsTest";
std::wstring CabFileName = L"WindowsFabricPackage.cab";
std::wstring OutputDirectory = L"Microsoft Service Fabric";

namespace Common
{
    class TestCabOperations
    {
    public:
        void CreateCabFile(bool addMumFile, wstring TestDirectory)
        {
            auto error = Directory::Create2(TestDirectory);
            VERIFY_IS_TRUE(error.IsSuccess());

            for (int i = 0; i < ARRAYSIZE(BinFileNamesArray); i++)
            {
                CreateFile(Path::Combine(TestDirectory, Path::Combine(PackageBaseDir, BinFileNamesArray[i])));
            }

            CreateFile(Path::Combine(TestDirectory, Path::Combine(PackageBaseDir, USFileName)));
            if (addMumFile)
            {
                CreateFile(Path::Combine(TestDirectory, Path::Combine(PackageBaseDir, UpdateFileName)));
            }

            HandleUPtr processHandle;
            HandleUPtr threadHandle;
            vector<wchar_t> envBlock(0);
            wstring workingDir = Path::Combine(TestDirectory, PackageBaseDir);
            auto makecab = wformatString("cabarc -r -p -m LZX:15 N ..\\{0} *", CabFileName);
            Trace.WriteInfo("Test", "Running command {0} from directory ", makecab, workingDir);
            error = ProcessUtility::CreateProcessW(makecab, workingDir, envBlock, 0, processHandle, threadHandle);
            VERIFY_IS_TRUE(error.IsSuccess(), wformatString("CreateProcess \"{0}\"; error: {1}", makecab, error).c_str());

            // Wait until child process exits.
            WaitForSingleObject(processHandle->Value, INFINITE);
        }

        void VerifyFileContent(wstring fileName, wstring expectedBaseContent, wstring TestDirectory)
        {
            File fileReader;

            auto error = fileReader.TryOpen(fileName, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
            VERIFY_IS_TRUE(error.IsSuccess(), wformatString("File {0} not opened because {1}", fileName, error).c_str());
            int64 fileSize = fileReader.size();
            size_t size = static_cast<size_t>(fileSize);
            wstring actualText;
            actualText.resize(size / 2);
            fileReader.Read(&actualText[0], static_cast<int>(size));
            fileReader.Close();
            StringUtility::TrimSpaces(actualText);
            wstring expectedText = Path::Combine(TestDirectory, Path::Combine(PackageBaseDir, expectedBaseContent));
            VERIFY_ARE_EQUAL(StringUtility::Compare(expectedText, actualText), 0, wformatString("{0}-{1}", actualText, expectedText).c_str());
        }

        void CreateFile(std::wstring fileName)
        {
            auto fileDir = Path::GetDirectoryName(fileName);
            auto error = Directory::Create2(fileDir);
            VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Unable to create {0}, because {1}", fileDir, error).c_str());
            FileWriter writer;
            error = writer.TryOpen(fileName);
            VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Unable to open {0}, because {1}", fileName, error).c_str());
            writer.WriteUnicodeBuffer(fileName.c_str(), fileName.length());
            writer.Close();
        }
    };


    BOOST_FIXTURE_TEST_SUITE(TestCabOperationsSuite, TestCabOperations)

    BOOST_AUTO_TEST_CASE(ExtractUpgradeServiceOutputDirectoryNotPresent) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"EUSODNP");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        std::wstring extractPath = Path::Combine(TestDirectory, OutputDirectory);
        std::vector<std::wstring> filters;
        filters.push_back(Path::GetDirectoryName(USFileName));
        CabOperations::ExtractFiltered(cabPath, extractPath, filters, true);
        auto expectedFile = Path::Combine(extractPath, USFileName);
        VERIFY_IS_TRUE(File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
        VerifyFileContent(expectedFile, USFileName, TestDirectory);
        for (int i = 0; i < ARRAYSIZE(BinFileNamesArray); i++)
        {
            expectedFile = Path::Combine(extractPath, BinFileNamesArray[i]);
            VERIFY_IS_TRUE(!File::Exists(expectedFile), wformatString("File {0} found", expectedFile).c_str());
        }
    }

    BOOST_AUTO_TEST_CASE(ExtractBinEULAOutputDirectoryNotPresent) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"EBEODNP");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        std::wstring extractPath = Path::Combine(TestDirectory, OutputDirectory);
        std::vector<std::wstring> filters;
        filters.push_back(L"bin");
        filters.push_back(L"EULA");
        CabOperations::ExtractFiltered(cabPath, extractPath, filters, true);
        auto expectedFile = Path::Combine(extractPath, USFileName);
        VERIFY_IS_TRUE(!File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
        for (int i = 0; i < ARRAYSIZE(BinFileNamesArray); i++)
        {
            expectedFile = Path::Combine(extractPath, BinFileNamesArray[i]);
            VERIFY_IS_TRUE(File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
            VerifyFileContent(expectedFile, BinFileNamesArray[i], TestDirectory);
        }
    }

    BOOST_AUTO_TEST_CASE(ExtractUpgradeServiceOutputDirectoryPresent) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"EUSODP");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        std::wstring extractPath = Path::Combine(TestDirectory, OutputDirectory);
        auto error = Directory::Create2(extractPath);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Directory {0} creation failed because {1}", extractPath, error).c_str());
        std::vector<std::wstring> filters;
        filters.push_back(Path::GetDirectoryName(USFileName));
        CabOperations::ExtractFiltered(cabPath, extractPath, filters, true);
        auto expectedFile = Path::Combine(extractPath, USFileName);
        VERIFY_IS_TRUE(File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
        VerifyFileContent(expectedFile, USFileName, TestDirectory);
        for (int i = 0; i < ARRAYSIZE(BinFileNamesArray); i++)
        {
            expectedFile = Path::Combine(extractPath, BinFileNamesArray[i]);
            VERIFY_IS_TRUE(!File::Exists(expectedFile), wformatString("File {0} found", expectedFile).c_str());
        }
    }

    BOOST_AUTO_TEST_CASE(ExtractBinEULAOutputDirectoryPresent) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"EBEODP");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        std::wstring extractPath = Path::Combine(TestDirectory, OutputDirectory);
        auto error = Directory::Create2(extractPath);
        VERIFY_IS_TRUE(error.IsSuccess(), wformatString("Directory {0} creation failed because {1}", extractPath, error).c_str());
        std::vector<std::wstring> filters;
        filters.push_back(L"bin");
        filters.push_back(L"EULA");
        CabOperations::ExtractFiltered(cabPath, extractPath, filters, true);
        auto expectedFile = Path::Combine(extractPath, USFileName);
        VERIFY_IS_TRUE(!File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
        for (int i = 0; i < ARRAYSIZE(BinFileNamesArray); i++)
        {
            expectedFile = Path::Combine(extractPath, BinFileNamesArray[i]);
            VERIFY_IS_TRUE(File::Exists(expectedFile), wformatString("File {0} not found", expectedFile).c_str());
            VerifyFileContent(expectedFile, BinFileNamesArray[i], TestDirectory);
        }
    }

    BOOST_AUTO_TEST_CASE(CheckMUMFileExistence) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CMFE");
        CreateCabFile(true, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        bool isFound;
        VERIFY_ARE_EQUAL(CabOperations::ContainsFile(cabPath, UpdateFileName, isFound), 0);
        VERIFY_IS_TRUE(isFound);
    }

    BOOST_AUTO_TEST_CASE(CheckMUMFileAbsence) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CMFA");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        bool isFound;
        VERIFY_ARE_EQUAL(CabOperations::ContainsFile(cabPath, UpdateFileName, isFound), 0);
        VERIFY_IS_TRUE(!isFound);
    }

    BOOST_AUTO_TEST_CASE(CheckIsCab) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CIS");
        CreateCabFile(false, TestDirectory);
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        bool isCab = CabOperations::IsCabFile(cabPath);
        VERIFY_IS_TRUE(isCab, wformatString("File {0} should have been CAB", cabPath).c_str());
    }

    BOOST_AUTO_TEST_CASE(CheckFileIsNotCab) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CINC");
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        CreateFile(cabPath);
        bool isCab = CabOperations::IsCabFile(cabPath);
        VERIFY_IS_FALSE(isCab, wformatString("File {0} is not CAB", cabPath).c_str());
    }

    BOOST_AUTO_TEST_CASE(CheckDirectoryIsNotCab) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CINC");
        std::wstring cabPath = Path::Combine(TestDirectory, CabFileName);
        CreateFile(cabPath);

        bool isCab = CabOperations::IsCabFile(TestDirectory);
        VERIFY_IS_FALSE(isCab, wformatString("Directory {0} is not CAB", TestDirectory).c_str());
    }

    BOOST_AUTO_TEST_CASE(CheckNonExistentFileIsNotCab) // Testcase1 is defined as a derived class of TestCaseBaseClass
    {
        wstring TestDirectory = Path::Combine(TestBaseDirectory, L"CINC");
        std::wstring cabPath = Path::Combine(TestDirectory, L"NotAFile.cab");

        bool isCab = CabOperations::IsCabFile(cabPath);
        VERIFY_IS_FALSE(isCab, wformatString("Nonexistent file {0} is not CAB", cabPath).c_str());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
