// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType("WorkingFolderTestHost");

GlobalWString WorkingFolderTestHost::WorkingFolderAppId = make_global<wstring>(L"WorkingFolderTestApp_App0");

WorkingFolderTestHost::WorkingFolderTestHost(wstring const & testName, bool isSetupEntryPoint)
    : testName_(testName),
    typeName_ (wformatString("{0}", testName_)),
    console_(),
    isSetupEntryPoint_(isSetupEntryPoint)
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
    auto hr = ::FabricGetActivationContext(
        IID_IFabricCodePackageActivationContext2,
        activationContext_.VoidInitializationAddress());    
    if (FAILED(hr))
    {
        WriteWarning(
            TraceType, 
            traceId_,
            "Failed to get CodePackageActivationContext. HRESULT={0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }

    auto error = this->VerifyWorkingFolder();    
    WriteTrace(
        error.ToLogLevel(),
        TraceType, 
        traceId_,
        "VerifyWorkingFolder: CurrentWorkingDirectory:{0}, Error={1}",
        Directory::GetCurrentDirectoryW(),
        hr);
    if(!error.IsSuccess())
    {
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
        WriteWarning(
            TraceType, 
            traceId_,
            "Failed to create FabricRuntime. HRESULT={0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }    

    WriteNoise(TraceType, traceId_, "FabricRuntime created.");
    console_.WriteLine("FabricRuntime created.");

    // create a service factory and register it for all the types
    ComPointer<IFabricStatelessServiceFactory> serviceFactory = make_com<ComStatelessServiceFactory, IFabricStatelessServiceFactory>();

    hr = runtime_->RegisterStatelessServiceFactory(typeName_.c_str(), serviceFactory.GetRawPointer());
    if (FAILED(hr))
    {
        WriteWarning(
        TraceType, 
        traceId_,
        "Failed to register stateless service factory for {1}. HRESULT={0}",
        hr,
        typeName_);
        return ErrorCode::FromHResult(hr);
    }

    console_.WriteLine("Registered ServiceFactory for type {0}", typeName_);

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
}

ErrorCode WorkingFolderTestHost::VerifyWorkingFolder()
{    
    ComPointer<IFabricCodePackage> codePackage;
    auto hr = activationContext_->GetCodePackage(testName_.c_str(), codePackage.InitializationAddress());
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
