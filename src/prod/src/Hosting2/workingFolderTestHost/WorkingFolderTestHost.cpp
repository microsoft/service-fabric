// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("WorkingFolderTestHost");

GlobalWString WorkingFolderTestHost::WorkingFolderAppId = make_global<wstring>(L"WorkingFolderTestApp_App0");

WorkingFolderTestHost::WorkingFolderTestHost(
    wstring const & testName, 
    bool isSetupEntryPoint,
    bool isActivator)
    : testName_(testName)
    , typeName_ (wformatString("{0}", testName_))
    , console_()
    , isSetupEntryPoint_(isSetupEntryPoint)
    , isActivator_(isActivator)
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));
    WriteNoise(TraceType, traceId_, "ctor");
}

WorkingFolderTestHost::~WorkingFolderTestHost()
{
    WriteNoise(TraceType, traceId_, "dtor");
}

ErrorCode WorkingFolderTestHost::OnOpen()
{
    EnvironmentMap envMap;
    if (!Environment::GetEnvironmentMap(envMap))
    {
        console_.WriteLine("Failed to obtain environment block.");
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    auto error = ApplicationHostContext::FromEnvironmentMap(envMap, hostContext_);
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "ApplicationHostContext::FromEnvironmentMap() failed. ErrorCode={0}, EnvironmentBlock={1}",
            error,
            Environment::ToString(envMap));

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    error = CodePackageContext::FromEnvironmentMap(envMap, codePackageContext_);
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "CodePackageContext::FromEnvironmentMap() failed. ErrorCode={0}, EnvironmentBlock={1}",
            error,
            Environment::ToString(envMap));
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    auto hr = ::FabricGetActivationContext(
        IID_IFabricCodePackageActivationContext2,
        activationContext_.VoidInitializationAddress());
    if (FAILED(hr))
    {
        auto err= ErrorCode::FromHResult(hr);
        console_.WriteLine("Failed to get CodePackageActivationContext. Error={0}", err);
        return err;
    }

    error = this->VerifyWorkingFolder();
    if(!error.IsSuccess())
    {
        console_.WriteLine(
            "VerifyWorkingFolder: CurrentWorkingDirectory:{0}, Error={1}",
            Directory::GetCurrentDirectoryW(),
            error);

        return error;
    }

    if(isSetupEntryPoint_) 
    {
        // Setup entry point does not register any ServiceType
        return ErrorCodeValue::Success;
    }
    
    hr = ::FabricCreateRuntime(IID_IFabricRuntime, runtime_.VoidInitializationAddress());
    if (FAILED(hr))
    {
        auto err = ErrorCode::FromHResult(hr);
        console_.WriteLine("Failed to create FabricRuntime. Error={0}.", err);
        return err;
    }

    console_.WriteLine("FabricRuntime created.");

    // create a service factory and register it for all the types
    auto serviceFactory = make_com<ComStatelessServiceFactory, IFabricStatelessServiceFactory>();

    hr = runtime_->RegisterStatelessServiceFactory(typeName_.c_str(), serviceFactory.GetRawPointer());
    if (FAILED(hr))
    {
        auto err = ErrorCode::FromHResult(hr);
        console_.WriteLine(
            "Failed to register stateless service factory for {1}. Error={0}.",
            typeName_,
            err);
        return err;
    }

    console_.WriteLine("Registered ServiceFactory for type {0}", typeName_);

    if (!isActivator_)
    {
        return ErrorCode::Success();
    }

    ComPointer<IFabricStringListResult> codePackageNameListCPtr;
    hr = activationContext_->GetCodePackageNames(codePackageNameListCPtr.InitializationAddress());
    if (FAILED(hr))
    {
        auto err = ErrorCode::FromHResult(hr);
        console_.WriteLine("Failed to get code package names. Error={0}.", err);
        return err;
    }

    FABRIC_STRING_LIST fabricStringList = {};
    hr = codePackageNameListCPtr->GetStrings(&fabricStringList.Count, &fabricStringList.Items);
    if (FAILED(hr))
    {
        auto err = ErrorCode::FromHResult(hr);
        console_.WriteLine(
            "codePackageNameListCPtr->GetStrings() failed. Error={0}",
            err);
        return err;
    }

    vector<wstring> codePackages;
    error = StringList::FromPublicApi(fabricStringList, codePackages);
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "StringList::FromPublicApi() failed. Error={0}.",
            error);
        return error;
    }

    for(auto const & cpName : codePackages)
    {
        if (cpName == codePackageContext_.CodePackageInstanceId.CodePackageName)
        {
            continue;
        }

        dependentCodePackages_.push_back(cpName);
    }

    activationManager_ = make_com<ComGuestServiceCodePackageActivationManager>(*this);
    error = activationManager_->Open();
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "Failed to open ComGuestServiceCodePackageActivationManager. Error={0}",
            error);

        //
        // return success to keep the service host up.
        // validation will fail i the test.
        //
        return ErrorCode::Success();
    }

    ::Sleep(15000);

    auto activationWaiter = make_shared<AsyncOperationWaiter>();

    EnvironmentMap environment;
    environment.insert(make_pair(L"DummyEnvVarName1", L"DummyEnvVarValue1"));
    environment.insert(make_pair(L"DummyEnvVarName2", L"DummyEnvVarValue2"));

    auto operation = activationManager_->BeginActivateCodePackagesAndWaitForEvent(
        move(environment),
        TimeSpan::FromMinutes(3),
        [this, activationWaiter](AsyncOperationSPtr const & operation)
        { 
            auto error = activationManager_->EndActivateCodePackagesAndWaitForEvent(operation);
            activationWaiter->SetError(error);
            activationWaiter->Set();
        },
        this->CreateAsyncOperationRoot());

    activationWaiter->WaitOne();
    error = activationWaiter->GetError();
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "Failed to activate dependent code packages. Error={0}",
            error);

        //
        // return success to keep the service host up.
        // validation will fail i the test.
        //
        return ErrorCode::Success();
    }

    ::Sleep(15000);

    auto deactivationWaiter = make_shared<AsyncOperationWaiter>();

    operation = activationManager_->BeginDeactivateCodePackages(
        TimeSpan::FromMinutes(3),
        [this, deactivationWaiter](AsyncOperationSPtr const & operation)
        {
            auto error = activationManager_->EndDeactivateCodePackages(operation);
            deactivationWaiter->SetError(error);
            deactivationWaiter->Set();
        },
        this->CreateAsyncOperationRoot());

    deactivationWaiter->WaitOne();
    error = deactivationWaiter->GetError();
    if (!error.IsSuccess())
    {
        console_.WriteLine(
            "Failed to deactivate dependent code packages. Error={0}",
            error);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode WorkingFolderTestHost::OnClose()
{
    OnAbort();
    return ErrorCode(ErrorCodeValue::Success);
}

void WorkingFolderTestHost::OnAbort()
{
    ComPointer<IFabricRuntime> runtimeCleanup = move(runtime_);
    if (runtimeCleanup.GetRawPointer() != NULL)
    {
        runtimeCleanup.Release();
    }

    ComPointer<IFabricCodePackageActivationContext2> contextCleanup = move(activationContext_);
    if(contextCleanup.GetRawPointer() != NULL)
    {
        contextCleanup.Release();
    }

    if (activationManager_)
    {
        activationManager_->Close();
        activationManager_.Release();
    }
}

ErrorCode WorkingFolderTestHost::GetCodePackageActivator(
    _Out_ ComPointer<IFabricCodePackageActivator> & codePackageActivator)
{
    auto hr = ::FabricGetCodePackageActivator(
        IID_IFabricCodePackageActivator, 
        codePackageActivator.VoidInitializationAddress());

    return ErrorCode::FromHResult(hr);
}

ErrorCode WorkingFolderTestHost::VerifyWorkingFolder()
{
    auto cpName = codePackageContext_.CodePackageInstanceId.CodePackageName;

    ComPointer<IFabricCodePackage> codePackage;
    auto hr = activationContext_->GetCodePackage(cpName.c_str(), codePackage.InitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceType, 
            traceId_,
            "Failed to get CodePackage. HRESULT={0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }    

    wstring program;
    FABRIC_EXEHOST_WORKING_FOLDER workingFolderType;
    auto description = codePackage->get_Description();
    if(isSetupEntryPoint_)
    {        
        workingFolderType = description->SetupEntryPoint->WorkingFolder;
        program = description->SetupEntryPoint->Program;
    }
    else
    {
        if(description->EntryPoint->Kind == FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST)
        {
            workingFolderType = ((FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION *)description->EntryPoint->Value)->WorkingFolder;
            program = ((FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION *)description->EntryPoint->Value)->Program;
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }
    
    wstring currentWorkingDirectory = Directory::GetCurrentDirectoryW();
    wstring workDirectory = wstring(activationContext_->get_WorkDirectory());

    console_.WriteLine("currentWorkingDirectory: [{0}]", currentWorkingDirectory);

#if !defined(PLATFORM_UNIX)
    Management::ImageModel::RunLayoutSpecification runLayout(Path::GetDirectoryName(Path::GetDirectoryName(workDirectory)));
#else
    Management::ImageModel::RunLayoutSpecification runLayout(Path::GetDirectoryName(Path::GetDirectoryName(workDirectory)));
#endif

    wstring serviceManifestName = description->ServiceManifestName;
    wstring codePackageName = description->Name;
    wstring codePackageVersion = description->Version;

    if(workingFolderType == FABRIC_EXEHOST_WORKING_FOLDER_WORK)
    {
        console_.WriteLine("if(FABRIC_EXEHOST_WORKING_FOLDER_WORK)");
        console_.WriteLine("WorkDirectory: [{0}]", workDirectory);

        if(!StringUtility::AreEqualCaseInsensitive(currentWorkingDirectory, workDirectory))
        {
            return ErrorCodeValue::InvalidDirectory;
        }
    }
    else if(workingFolderType == FABRIC_EXEHOST_WORKING_FOLDER_CODE_PACKAGE)
    {        
        console_.WriteLine("if(FABRIC_EXEHOST_WORKING_FOLDER_CODE_PACKAGE)");
        
        wstring codePackageDirectory = runLayout.GetCodePackageFolder(
            *WorkingFolderAppId,
            serviceManifestName,
            codePackageName,
            codePackageVersion,
            false);

        console_.WriteLine("codePackageDirectory: [{0}]", codePackageDirectory);

        if(!StringUtility::AreEqualCaseInsensitive(currentWorkingDirectory, codePackageDirectory))
        {
            return ErrorCodeValue::InvalidDirectory;
        }
    }
    else if(workingFolderType == FABRIC_EXEHOST_WORKING_FOLDER_CODE_BASE)
    {
        console_.WriteLine("if(FABRIC_EXEHOST_WORKING_FOLDER_CODE_BASE)");

        wstring codePackageDirectory = runLayout.GetCodePackageFolder(
            *WorkingFolderAppId,
            serviceManifestName,
            codePackageName,
            codePackageVersion,
            false);

        wstring fullProgramPath = Path::Combine(codePackageDirectory, program);
        wstring codeBaseDirectory = Path::GetDirectoryName(fullProgramPath);

        console_.WriteLine("codeBaseDirectory: [{0}]", codeBaseDirectory);

        if(!StringUtility::AreEqualCaseInsensitive(currentWorkingDirectory, codeBaseDirectory))
        {
            return ErrorCodeValue::InvalidDirectory;
        }   
    }
    else
    {
        console_.WriteLine("else(Invalid Working Folder value)");

        Assert::CodingError("Invalid Working Folder value.");
    }

    return ErrorCodeValue::Success;
}
