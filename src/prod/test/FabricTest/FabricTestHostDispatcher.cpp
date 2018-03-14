// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const TraceSource("FabricTestHost.Dispatcher");

wstring const FabricTestHostDispatcher::ParamDelimiter = L" ";
wstring const FabricTestHostDispatcher::StatePropertyDelimiter = L".";

class TestPackageChangeHandler
{
public:
	TestPackageChangeHandler(TestCodePackageContext const& context, shared_ptr<ServiceManifestDescription> const& serviceManifestSPtr)
		: context_(context)
		, servicePackageVersion_(serviceManifestSPtr->Version)
		, lock_()
	{
		serviceManifests_.push_back(serviceManifestSPtr);
	}

	template<typename TPackage, typename TServiceModelPackageDescription, typename TPublicPackageDescription>
	void ValidateEventAndSendUpdatedManifestDescription(IFabricCodePackageActivationContext * source, TPackage* oldPackage, TPackage* newPackage, std::function<const vector<TServiceModelPackageDescription>* (ServiceManifestDescription const &)> packageGetter)
	{
		AcquireExclusiveLock grab(lock_);
		ComPointer<Hosting2::ComCodePackageActivationContext> codePackageActivationContext;
		codePackageActivationContext.SetAndAddRef(static_cast<Hosting2::ComCodePackageActivationContext*>(source));
		shared_ptr<ServiceManifestDescription> serviceManifestSPtr;
		codePackageActivationContext->Test_CodePackageActivationContextObj.GetServiceManifestDescription(serviceManifestSPtr);
                TestSession::WriteInfo(TraceSource, "Existing service manifest version {0}", servicePackageVersion_);
		if (serviceManifestSPtr->Version != servicePackageVersion_)
		{
                        TestSession::WriteInfo(TraceSource, "New service manifest version is {0}. Sending update", serviceManifestSPtr->Version);
			servicePackageVersion_ = serviceManifestSPtr->Version;
			FABRICHOSTSESSION.Dispatcher.SendServiceManifestUpdate(
				*serviceManifestSPtr,
				context_);
		}

		// add this manifest to the set of manifests we have seen if not already added
		// thus if this is the first notification of upgrade from 1.0 to 1.1 then the vector
		// contains only the manifest for 1.0 and 1.1 will get added to it
		// if this is the second notification then the vector already contains 1.0, 1.1 so no need to add
		if ((serviceManifests_.end() - 1)->get()->Version != serviceManifestSPtr->Version)
		{
			serviceManifests_.push_back(serviceManifestSPtr);
		}
		else
		{
			TestSession::FailTestIf(**(serviceManifests_.end() - 1) != *serviceManifestSPtr, "Service manifests of same version do not match");
		}

		ServiceManifestDescription oldManifest;
		ServiceManifestDescription newManifest;
		if (serviceManifests_.size() > 1)
		{
			oldManifest = **(serviceManifests_.end() - 2);
			newManifest = **(serviceManifests_.end() - 1);
		}
		else
		{
			oldManifest = **(serviceManifests_.end() - 1);
			newManifest = **(serviceManifests_.end() - 1);
		}

		if (oldPackage == nullptr && newPackage == nullptr)
		{
			TestSession::FailTest("old package and new package were null in package changed");
		}
		else if (oldPackage != nullptr && newPackage == nullptr)
		{
			// if the old package is not null then it should exist in the old manifest and not in new
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(true, oldPackage, oldManifest, packageGetter);
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(false, oldPackage, newManifest, packageGetter);
		}
		else if (oldPackage == nullptr && newPackage != nullptr)
		{
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(false, newPackage, oldManifest, packageGetter);
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(true, newPackage, newManifest, packageGetter);
		}
		else if (oldPackage != nullptr && newPackage != nullptr)
		{
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(true, newPackage, oldManifest, packageGetter);
			AssertPackageInManifest<TPackage, TServiceModelPackageDescription, TPublicPackageDescription>(true, newPackage, newManifest, packageGetter);
		}
	}

	template<typename TPackage, typename TServiceModelPackageDescription, typename TPublicPackageDescription>
	void AssertPackageInManifest(bool expected, TPackage* package, ServiceManifestDescription const & manifest, std::function<const std::vector<TServiceModelPackageDescription>* (ServiceManifestDescription const &)> packageGetter)
	{
		wstring packageName;
		HRESULT hr = S_OK;

		hr = GetName<TPackage, TPublicPackageDescription>(package, packageName);
		TestSession::FailTestIf(hr != S_OK, "unable to read a package name");

		auto packages = packageGetter(manifest);
		auto findIter = find_if(packages->begin(), packages->end(), [&](TServiceModelPackageDescription const & pkg)
		{
			return StringUtility::AreEqualCaseInsensitive(packageName, pkg.Name);
		});

		bool isPackageInManifest = findIter != packages->end();
		TestSession::WriteInfo(TraceSource, "Validating package presence for {0}", packageName);
		TestSession::FailTestIf(expected != isPackageInManifest, "A package was {0} to be in manifest but actually {1} in manifest", expected, isPackageInManifest);
	}

private:
	template<typename TPackage, typename TPublicPackageDescription>
	HRESULT GetName(TPackage* package, wstring& name)
	{
		HRESULT hr = S_OK;

		const TPublicPackageDescription* packageDescription = package->get_Description();
		name = wstring(packageDescription->Name);
		return hr;
	}

	TestCodePackageContext context_;
	wstring servicePackageVersion_;
	ExclusiveLock lock_;
	vector<shared_ptr<ServiceManifestDescription>> serviceManifests_;
};

class ComCodePackageChangeHandler :
	public IFabricCodePackageChangeHandler,
	private ComUnknownBase
{
	DENY_COPY(ComCodePackageChangeHandler);

	BEGIN_COM_INTERFACE_LIST(ComCodePackageChangeHandler)
		COM_INTERFACE_ITEM(IID_IFabricCodePackageChangeHandler, IFabricCodePackageChangeHandler)
    END_COM_INTERFACE_LIST()

public:
	ComCodePackageChangeHandler(shared_ptr<TestPackageChangeHandler> const& packageChangeHandlerSPtr)
		: packageChangeHandlerSPtr_(packageChangeHandlerSPtr)
	{
	}

	virtual void STDMETHODCALLTYPE OnPackageAdded(
		IFabricCodePackageActivationContext * source,
		IFabricCodePackage * newPackage)
	{
		this->OnPackageModified(source, nullptr, newPackage);
	}

	virtual void STDMETHODCALLTYPE OnPackageRemoved(
		IFabricCodePackageActivationContext * source,
		IFabricCodePackage * oldPackage)
	{
		this->OnPackageModified(source, oldPackage, nullptr);
	}

	virtual void STDMETHODCALLTYPE OnPackageModified(
		IFabricCodePackageActivationContext * source,
		IFabricCodePackage * oldPackage,
		IFabricCodePackage * newPackage)
	{
		auto packageGetter = [](ServiceManifestDescription const & manifest)
		{
			return &manifest.CodePackages;
		};

		packageChangeHandlerSPtr_->ValidateEventAndSendUpdatedManifestDescription<IFabricCodePackage, CodePackageDescription, FABRIC_CODE_PACKAGE_DESCRIPTION>(source, oldPackage, newPackage, packageGetter);
	}

private:
	shared_ptr<TestPackageChangeHandler> packageChangeHandlerSPtr_;
};

class ComConfigPackageChangeHandler :
	public IFabricConfigurationPackageChangeHandler,
	private ComUnknownBase
{
	DENY_COPY(ComConfigPackageChangeHandler);

	BEGIN_COM_INTERFACE_LIST(ComConfigPackageChangeHandler)
		COM_INTERFACE_ITEM(IID_IFabricConfigurationPackageChangeHandler, IFabricConfigurationPackageChangeHandler)
		END_COM_INTERFACE_LIST()

public:
	ComConfigPackageChangeHandler(shared_ptr<TestPackageChangeHandler> const& packageChangeHandlerSPtr)
		: packageChangeHandlerSPtr_(packageChangeHandlerSPtr)
	{
	}

	virtual void STDMETHODCALLTYPE OnPackageAdded(
		IFabricCodePackageActivationContext * source,
		IFabricConfigurationPackage * newPackage)
	{
		this->OnPackageModified(source, nullptr, newPackage);
	}

	virtual void STDMETHODCALLTYPE OnPackageRemoved(
		IFabricCodePackageActivationContext * source,
		IFabricConfigurationPackage * oldPackage)
	{
		this->OnPackageModified(source, oldPackage, nullptr);
	}

	virtual void STDMETHODCALLTYPE OnPackageModified(
		IFabricCodePackageActivationContext * source,
		IFabricConfigurationPackage * oldPackage,
		IFabricConfigurationPackage * newPackage)
	{
		auto packageGetter = [](ServiceManifestDescription const & manifest)
		{
			return &manifest.ConfigPackages;
		};

		packageChangeHandlerSPtr_->ValidateEventAndSendUpdatedManifestDescription<IFabricConfigurationPackage, ConfigPackageDescription, FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION>(source, oldPackage, newPackage, packageGetter);
	}

private:
	shared_ptr<TestPackageChangeHandler> packageChangeHandlerSPtr_;
};

class ComDataPackageChangeHandler :
	public IFabricDataPackageChangeHandler,
	private ComUnknownBase
{
	DENY_COPY(ComDataPackageChangeHandler);

	BEGIN_COM_INTERFACE_LIST(ComDataPackageChangeHandler)
		COM_INTERFACE_ITEM(IID_IFabricDataPackageChangeHandler, IFabricDataPackageChangeHandler)
		END_COM_INTERFACE_LIST()

public:
	ComDataPackageChangeHandler(shared_ptr<TestPackageChangeHandler> const& packageChangeHandlerSPtr)
		: packageChangeHandlerSPtr_(packageChangeHandlerSPtr)
	{
	}

	virtual void STDMETHODCALLTYPE OnPackageAdded(
		IFabricCodePackageActivationContext * source,
		IFabricDataPackage * newPackage)
	{
		this->OnPackageModified(source, nullptr, newPackage);
	}

	virtual void STDMETHODCALLTYPE OnPackageRemoved(
		IFabricCodePackageActivationContext * source,
		IFabricDataPackage * oldPackage)
	{
		this->OnPackageModified(source, oldPackage, nullptr);
	}

	virtual void STDMETHODCALLTYPE OnPackageModified(
		IFabricCodePackageActivationContext * source,
		IFabricDataPackage * oldPackage,
		IFabricDataPackage * newPackage)
	{
		auto packageGetter = [](ServiceManifestDescription const & manifest)
		{
			return &manifest.DataPackages;
		};

		packageChangeHandlerSPtr_->ValidateEventAndSendUpdatedManifestDescription<IFabricDataPackage, DataPackageDescription, FABRIC_DATA_PACKAGE_DESCRIPTION>(source, oldPackage, newPackage, packageGetter);
	}

private:
	shared_ptr<TestPackageChangeHandler> packageChangeHandlerSPtr_;
};

FabricTestHostDispatcher::FabricTestHostDispatcher()
    : CommonDispatcher(),
    hostType_(ApplicationHostType::NonActivated),
    codePackages_(),
    singleHostCodePackage_()
{
}

bool FabricTestHostDispatcher::Open()
{
    if(hostType_ == ApplicationHostType::Activated_SingleCodePackage)
    {
		// initialServiceManifestSPtr used to setup the code/config/data change handlers. There is a possible race where as soon as the process comes 
		// up in version A there is an update to version B of the service manifest. To avoid this race we first register the change hendlers
		// and after the registrations which guarantees the notification we get a new service manifest description (this is done below after registration and create runtime)
		// to send the update back to the main FabricTest.exe process about the current state. This avoids sending the stale update
        shared_ptr<ServiceManifestDescription> initialServiceManifestSPtr;
		auto error = singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.GetServiceManifestDescription(initialServiceManifestSPtr);
        TestSession::FailTestIfNot(error.IsSuccess(), "GetServiceManifestDescription failed with {0}", error);
		auto changeHandlerSPtr = make_shared<TestPackageChangeHandler>(
            singleHostCodePackage_.TestCodePackageContext,
			initialServiceManifestSPtr);

		auto codeChangeHandlerCPtr = make_com<ComCodePackageChangeHandler>(changeHandlerSPtr);
		auto configChangeHandlerCPtr = make_com<ComConfigPackageChangeHandler>(changeHandlerSPtr);
		auto dataChangeHandlerCPtr = make_com<ComDataPackageChangeHandler>(changeHandlerSPtr);


		HRESULT hr = singleHostCodePackage_.CodePackageActivationContextCPtr->RegisterCodePackageChangeHandler(codeChangeHandlerCPtr.GetRawPointer(), &singleHostCodePackage_.CodeHandlerId);
        TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterCodePackageChangeHandler failed with {0}", hr);
		hr = singleHostCodePackage_.CodePackageActivationContextCPtr->RegisterConfigurationPackageChangeHandler(configChangeHandlerCPtr.GetRawPointer(), &singleHostCodePackage_.ConfigHandlerId);
        TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterConfigurationPackageChangeHandler failed with {0}", hr);
		hr = singleHostCodePackage_.CodePackageActivationContextCPtr->RegisterDataPackageChangeHandler(dataChangeHandlerCPtr.GetRawPointer(), &singleHostCodePackage_.DataHandlerId);
        TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterDataPackageChangeHandler failed with {0}", hr);

        hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, singleHostCodePackage_.FabricRuntime.VoidInitializationAddress());
        TestSession::FailTestIf(FAILED(hr), "ApplicationHostContainer::FabricCreateRuntime");

        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "Created runtime....");

        singleHostCodePackage_.TestHostServiceFactory = make_shared<TestHostServiceFactory>(
            FABRICHOSTSESSION.ClientId, 
            singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.Test_CodePackageId.CodePackageName,
            singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.Test_CodePackageId.ServicePackageInstanceId.ApplicationId.ToString(),
            singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.Test_CodePackageId.ServicePackageInstanceId.ServicePackageName,
            singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.WorkDirectory,
            clientSecuritySettings_); 

		shared_ptr<ServiceManifestDescription> serviceManifestSPtr;
		error = singleHostCodePackage_.CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.GetServiceManifestDescription(serviceManifestSPtr);
		TestSession::FailTestIfNot(error.IsSuccess(), "GetServiceManifestDescription failed with {0}", error);

		SendServiceManifestUpdate(*serviceManifestSPtr, singleHostCodePackage_.TestCodePackageContext);

        RegisterServiceTypes(singleHostCodePackage_.TestCodePackageContext.CodePackageId.CodePackageName, singleHostCodePackage_.FabricRuntime, singleHostCodePackage_.CodePackageActivationContextCPtr, singleHostCodePackage_.TestHostServiceFactory);

        ValidateErrorCodes(*singleHostCodePackage_.CodePackageActivationContextCPtr.GetRawPointer());
    }
    else if(hostType_ == ApplicationHostType::Activated_MultiCodePackage)
    {
        ComPointer<ComCodePackageHost> comCodePackageHostCPtr = make_com<ComCodePackageHost>(hostId_);
        ApplicationHostContainer::FabricRegisterCodePackageHost(comCodePackageHostCPtr.GetRawPointer());
    }
    else 
    {
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "Unknown host type {0}", hostType_);
        return false;
    }

    SendPeriodicPlacementData();

    return true;
}

void FabricTestHostDispatcher::SendPeriodicPlacementData()
{
    GetPlacementData(StringCollection());

    auto root = FABRICHOSTSESSION.CreateComponentRoot();
    Threadpool::Post([this, root] () 
    { 
        SendPeriodicPlacementData();
    }, TimeSpan::FromSeconds(60));
}

void FabricTestHostDispatcher::ValidateErrorCodes(IFabricCodePackageActivationContext& activationContext)
{
    // generate a guid
    Common::Guid guid = Guid::NewGuid();

    wstring strGuid = guid.ToString();

    // there can't be a package by this name
    // assert that the correct error code is returned
    HRESULT hr = S_OK;

    IFabricCodePackage* pCodePackage = nullptr;
    IFabricDataPackage* pDataPackage = nullptr;
    IFabricConfigurationPackage* pConfigPackage = nullptr;

    hr = activationContext.GetCodePackage(strGuid.c_str(), &pCodePackage);
    TestSession::FailTestIf(hr != FABRIC_E_CODE_PACKAGE_NOT_FOUND, "incorrect error code for non-existent codepackage");

    hr = activationContext.GetDataPackage(strGuid.c_str(), &pDataPackage);
    TestSession::FailTestIf(hr != FABRIC_E_DATA_PACKAGE_NOT_FOUND, "incorrect error code for non-existent data package");

    hr = activationContext.GetConfigurationPackage(strGuid.c_str(), &pConfigPackage);
    TestSession::FailTestIf(hr != FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND, "incorrect error code for non-existent config package");
}

StringCollection CompactParameters(StringCollection const & params)
{
    StringCollection compactedParams;
    bool foundStartingQuote = false;
    wstring currentParam;
    for (wstring const & nodeIdString : params)
    {
        if (foundStartingQuote)
        {
            if (nodeIdString[nodeIdString.size() - 1] == L'"')
            {
                foundStartingQuote = false;
                currentParam += L" " + nodeIdString.substr(0, nodeIdString.size() - 1);
                compactedParams.push_back(currentParam);
            }
            else
            {
                currentParam += L" " + nodeIdString;
            }
        }
        else if (nodeIdString[0] == L'"')
        {
            if (nodeIdString[nodeIdString.size() - 1] == L'"')
            {
                compactedParams.push_back(nodeIdString.substr(1, nodeIdString.size() - 2));
            }
            else
            {
                foundStartingQuote = true;
                currentParam = nodeIdString.substr(1, nodeIdString.size() - 1);
            }
        }
        else
        {
            compactedParams.push_back(nodeIdString);
        }
    }
    return compactedParams;
}

bool FabricTestHostDispatcher::ExecuteCommand(wstring command)
{
    StringCollection paramCollection;

    StringUtility::Split<wstring>(command, paramCollection, FabricTestHostDispatcher::ParamDelimiter);
    if (paramCollection.size() == 0)
    {
        return false;
    }

    command = *paramCollection.begin();
    paramCollection.erase(paramCollection.begin());
    paramCollection = CompactParameters(paramCollection);

    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "Processing command {0}", command);
    if(command == FabricTestCommands::GetPlacementDataCommand)
    {
        return GetPlacementData(paramCollection);
    }
    else if(command == FabricTestCommands::KillCodePackageCommand)
    {
        return KillCodePackage(paramCollection);
    }
    else if(command == FabricTestCommands::FailFastCommand)
    {
        Assert::CodingError("fail Command at {0}", FABRICHOSTSESSION.ClientId);
    }
    else if(command == FabricTestCommands::PutDataCommand)
    {
        return PutData(paramCollection);
    }
    else if(command == FabricTestCommands::GetDataCommand)
    {
        return GetData(paramCollection);
    }
    else
    {
        return false;
    }
}

void FabricTestHostDispatcher::PrintHelp(std::wstring const & command)
{
    FABRICHOSTSESSION.PrintHelp(command);
}

MessageId MessageIdFromString(wstring const & input)
{
    vector<wstring> values;
    StringUtility::Split<wstring>(input, values, L":");
    TestSession::FailTestIfNot(values.size() == 2, "Incorrect message id string {0}", input);
    DWORD index;
    TestSession::FailTestIfNot(StringUtility::TryFromWString(values[1], index), "Could not parse as DWORD {0}", values[1]);
    return MessageId(Guid(values[0]), index);
}

bool FabricTestHostDispatcher::PutData(Common::StringCollection const & params)
{
    TestSession::FailTestIfNot(params.size() == 6, "Incorrect arguments for PutData {0}", params);

    auto root = FABRICHOSTSESSION.CreateComponentRoot();
    Threadpool::Post([params, root, this]()
    {
        __int64 key = 0;
        TestSession::FailTestIfNot(StringUtility::TryFromWString(params[0], key), "Could not parse as __int64 {0}", params[0]);
        wstring value = params[1];
        FABRIC_SEQUENCE_NUMBER currentVersion = 0;
        TestSession::FailTestIfNot(StringUtility::TryFromWString(params[2], currentVersion), "Could not parse as FABRIC_SEQUENCE_NUMBER {0}", params[2]);
        wstring serviceName = params[3];
        wstring serviceLocation = params[4];
        MessageId messageId = MessageIdFromString(params[5]);

        ITestStoreServiceSPtr storeServiceSPtr;
        FABRIC_SEQUENCE_NUMBER newVersion = -1;
        ULONGLONG dataLossVersion = 0;
        ErrorCode error;
        if(TryFindStoreService(serviceLocation, storeServiceSPtr))
        {
            error = storeServiceSPtr->Put(
                key,
                value,
                currentVersion,
                serviceName,
                newVersion,
                dataLossVersion);
        }
        else
        {
            error = ErrorCode(ErrorCodeValue::NotFound);
        }

        wstring replyString = wformatString("{0} {1}", newVersion, dataLossVersion);
        SendClientCommandReply(
            replyString, 
            error.ReadValue(),
            messageId);
    });

    return true;
}

bool FabricTestHostDispatcher::GetData(Common::StringCollection const & params)
{
    TestSession::FailTestIfNot(params.size() == 4, "Incorrect arguments for PutData {0}", params);

    auto root = FABRICHOSTSESSION.CreateComponentRoot();
    Threadpool::Post([params, root, this]()
    {
        __int64 key = 0;
        TestSession::FailTestIfNot(StringUtility::TryFromWString(params[0], key), "Could not parse as __int64 {0}", params[0]);
        wstring serviceName = params[1];
        wstring serviceLocation = params[2];
        MessageId messageId = MessageIdFromString(params[3]);

        ITestStoreServiceSPtr storeServiceSPtr;
        wstring value(L"Invalid");
        FABRIC_SEQUENCE_NUMBER version = -1;
        ULONGLONG dataLossVersion = 0;
        ErrorCode error;
        if(TryFindStoreService(serviceLocation, storeServiceSPtr))
        {
            error = storeServiceSPtr->Get(
                key,
                serviceName,
                value,
                version,
                dataLossVersion);
        }
        else
        {
            error = ErrorCode(ErrorCodeValue::NotFound);
        }

        value = value.empty() ? L"Empty" : value;
        wstring replyString = wformatString("{0} {1} {2}", value, version, dataLossVersion);
        SendClientCommandReply(
            replyString, 
            error.ReadValue(),
            messageId);
    });

    return true;
}

bool FabricTestHostDispatcher::TryFindStoreService(wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr)
{

    if(hostType_ == ApplicationHostType::Activated_SingleCodePackage)
    {
        return singleHostCodePackage_.TestHostServiceFactory->TryFindStoreService(serviceLocation, storeServiceSPtr);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "TryFindStoreService");
        AcquireExclusiveLock grab(codePackagesLock_);
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "TryFindStoreService acquiredLock");
        for (auto iter = codePackages_.begin(); iter != codePackages_.end(); ++iter)
        {
            shared_ptr<TestCodePackage> const& testCodePackage = iter->second;
            if(testCodePackage->TestHostServiceFactory->TryFindStoreService(serviceLocation, storeServiceSPtr))
            {
                return true;
            }
        }
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "TryFindStoreService releasing lock");
        return false;
    }
}

bool FabricTestHostDispatcher::GetPlacementData(Common::StringCollection const &)
{
    if(hostType_ == ApplicationHostType::Activated_SingleCodePackage)
    {
        SendPlacementData(
            singleHostCodePackage_.TestHostServiceFactory->ActiveServices, 
            singleHostCodePackage_.TestCodePackageContext);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "GetPlacementData");
        AcquireExclusiveLock grab(codePackagesLock_);
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "GetPlacementData acquiredlock");
        for (auto iter = codePackages_.begin(); iter != codePackages_.end(); ++iter)
        {
            shared_ptr<TestCodePackage> const& testCodePackage = iter->second;

            SendPlacementData(
                testCodePackage->TestHostServiceFactory->ActiveServices, 
                testCodePackage->TestCodePackageContext);
        }
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "GetPlacementData releasing lock");
    }

    return true;
}

void FabricTestHostDispatcher::SendPlacementData(std::map<std::wstring, TestServiceInfo> const& activeServices, wstring const& codePackageId)
{
    if(hostType_ == ApplicationHostType::Activated_SingleCodePackage)
    {
        SendPlacementData(
            activeServices, 
            singleHostCodePackage_.TestCodePackageContext);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SendPlacementData");
        AcquireExclusiveLock grab(codePackagesLock_);
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SendPlacementData acquiredlock");
        for (auto iter = codePackages_.begin(); iter != codePackages_.end(); ++iter)
        {
            if(iter->first == codePackageId)
            {
                shared_ptr<TestCodePackage> const& testCodePackage = iter->second;

                SendPlacementData(
                    activeServices, 
                    testCodePackage->TestCodePackageContext);
            }
        }
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SendPlacementData releasing lock");
    }
}

void FabricTestHostDispatcher::SendPlacementData(
    map<wstring, TestServiceInfo> const& activeServices, 
    TestCodePackageContext const& testCodePackageContext)
{
    ServicePlacementData servicePlacementData(
        activeServices, 
        testCodePackageContext);
    vector <byte> data;
    ErrorCode error = FabricSerializer::Serialize(&servicePlacementData, data);
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to serialize service placement data due to {0}", error);
    FABRICHOSTSESSION.ReportClientData(*MessageType::ServicePlacementData, move(data));
}

void FabricTestHostDispatcher::SendServiceManifestUpdate(
    ServiceManifestDescription const& description, 
    TestCodePackageContext const& testCodePackageContext)
{
    TestCodePackageData codePackageData(
        description, 
        testCodePackageContext);
    vector <byte> data;
    ErrorCode error = FabricSerializer::Serialize(&codePackageData, data);
    TestSession::FailTestIfNot(error.IsSuccess(), "Manifest update: Failed to serialize code package data due to {0}", error);
    FABRICHOSTSESSION.ReportClientData(*MessageType::ServiceManifestUpdate, move(data));
}

void FabricTestHostDispatcher::SendCodePackageActivate(
    ServiceManifestDescription const& description, 
    TestCodePackageContext const& testCodePackageContext)
{
    TestCodePackageData codePackageData(
        description, 
        testCodePackageContext);
    vector <byte> data;
    ErrorCode error = FabricSerializer::Serialize(&codePackageData, data);
    TestSession::FailTestIfNot(error.IsSuccess(), "Code package activate: Failed to serialize code package data due to {0}", error);
    FABRICHOSTSESSION.ReportClientData(*MessageType::CodePackageActivate, move(data));
}

void FabricTestHostDispatcher::SendClientCommandReply(
    wstring const& replyString, 
    ErrorCodeValue::Enum errorCodeValue,
    MessageId const& messageId)
{
    ClientCommandReply clientCommandReply(
        replyString, 
        errorCodeValue,
        messageId);
    vector <byte> data;
    ErrorCode error = FabricSerializer::Serialize(&clientCommandReply, data);
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to serialize client command reply data due to {0}", error);
    FABRICHOSTSESSION.ReportClientData(*MessageType::ClientCommandReply, move(data));
}

bool FabricTestHostDispatcher::KillCodePackage(Common::StringCollection const & params)
{
    if(hostType_ == ApplicationHostType::Activated_SingleCodePackage)
    {
        //Since single code package we exit process
        ExitProcess(1);
    }
    else if (hostType_ == ApplicationHostType::Activated_MultiCodePackage)
    {
        CommandLineParser parser(params);
        wstring codePackageIdentifier;
        parser.TryGetString(L"cp", codePackageIdentifier, L"");
        bool killDllHost = parser.GetBool(L"killhost");
        if(killDllHost)
        {
            ExitProcess(1);
        }
        else
        {
            DeactivateCodePackage(codePackageIdentifier);
        }
    }

    return true;
}

wstring FabricTestHostDispatcher::GetState(std::wstring const & param)
{
    vector<wstring> params;
    StringUtility::Split<wstring>(param, params, StatePropertyDelimiter);
    return wstring();
}

ErrorCode FabricTestHostDispatcher::InitializeClientSecuritySettings(SecurityProvider::Enum provider)
{
    bool result = true;
    wstring unusedCredentialsType;
    wstring errorMessage;

    // This must match the logic in TestMain.cpp that loads
    // hardcoded client security credentials for use in 
    // random FabricTest runs.
    //
    if (provider == Transport::SecurityProvider::Ssl)
    {
        result = ClientSecurityHelper::TryCreateClientSecuritySettings(
            ClientSecurityHelper::DefaultSslClientCredentials,
            unusedCredentialsType, // out
            clientSecuritySettings_, // out
            errorMessage); // out
    }
    else if (SecurityProvider::IsWindowsProvider(provider))
    {
        result = ClientSecurityHelper::TryCreateClientSecuritySettings(
            ClientSecurityHelper::DefaultWindowsCredentials,
            unusedCredentialsType, // out
            clientSecuritySettings_, // out
            errorMessage); // out
    }

    if (!result)
    {
        TestSession::WriteInfo(TraceSource, "{0}: SetClientCredentials failed: {1}", __FUNCTION__, errorMessage);
        return ErrorCode(ErrorCodeValue::InvalidCredentials, move(errorMessage));
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "{0}: SetClientCredentials result: {1}", __FUNCTION__, clientSecuritySettings_);
        return ErrorCodeValue::Success;
    }
}

void FabricTestHostDispatcher::SetAsSinglePackageHost(ComPointer<ComCodePackageActivationContext> const& comCodePackageActivationContextCPtr, TestCodePackageContext const& testCodePackageContext)
{
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SetAsSinglePackageHost {0}", testCodePackageContext.CodePackageId);
    TestSession::FailTestIfNot(hostType_ == ApplicationHostType::NonActivated, "SetAsSinglePackageHost when host type is not invalid");
    hostType_ = ApplicationHostType::Activated_SingleCodePackage;

    singleHostCodePackage_.CodePackageActivationContextCPtr = comCodePackageActivationContextCPtr;
    singleHostCodePackage_.TestCodePackageContext = testCodePackageContext;
}

void FabricTestHostDispatcher::SetAsMultiPackageHost(TestMultiPackageHostContext const& testMultiPackageHostContext)
{
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SetAsMultiPackageHost");
    TestSession::FailTestIfNot(hostType_ == ApplicationHostType::NonActivated, "SetAsSinglePackageHost when host type is not invalid");
    hostType_ = ApplicationHostType::Activated_MultiCodePackage;
    hostId_ = testMultiPackageHostContext.HostId;
}

void FabricTestHostDispatcher::ActivateCodePackage(
    std::wstring const& codePackageId,
    Common::ComPointer<Hosting2::ComCodePackageActivationContext> const& activationContext, 
    Common::ComPointer<IFabricRuntime> const& fabricRuntime,
    TestCodePackageContext const& testCodePackageContext)
{
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "ActivateCodePackage {0}", codePackageId);
    AcquireExclusiveLock grab(codePackagesLock_);
    TestSession::WriteNoise(TraceSource, FABRICHOSTSESSION.ClientId, "ActivateCodePackage {0} acquiredlock", codePackageId);
    shared_ptr<TestCodePackage> testCodePackage = make_shared<TestCodePackage>();
    testCodePackage->CodePackageActivationContextCPtr = activationContext;
    testCodePackage->FabricRuntime = fabricRuntime;
    testCodePackage->TestCodePackageContext = testCodePackageContext;

    testCodePackage->TestHostServiceFactory = make_shared<TestHostServiceFactory>(
        FABRICHOSTSESSION.ClientId, 
        codePackageId,
        activationContext->Test_CodePackageActivationContextObj.Test_CodePackageId.ServicePackageInstanceId.ApplicationId.ToString(),
        activationContext->Test_CodePackageActivationContextObj.Test_CodePackageId.ServicePackageInstanceId.ServicePackageName,
        activationContext->Test_CodePackageActivationContextObj.WorkDirectory,
        clientSecuritySettings_); 

    shared_ptr<ServiceManifestDescription> serviceManifestSPtr;
    auto error = testCodePackage->CodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.GetServiceManifestDescription(serviceManifestSPtr);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetServiceManifestDescription failed with {0}", error);
	auto changeHandlerSPtr = make_shared<TestPackageChangeHandler>(
		singleHostCodePackage_.TestCodePackageContext,
		serviceManifestSPtr);

	auto codeChangeHandlerCPtr = make_com<ComCodePackageChangeHandler>(changeHandlerSPtr);
	auto configChangeHandlerCPtr = make_com<ComConfigPackageChangeHandler>(changeHandlerSPtr);
	auto dataChangeHandlerCPtr = make_com<ComDataPackageChangeHandler>(changeHandlerSPtr);

	HRESULT hr = testCodePackage->CodePackageActivationContextCPtr->RegisterCodePackageChangeHandler(codeChangeHandlerCPtr.GetRawPointer(), &testCodePackage->CodeHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterCodePackageChangeHandler failed with {0}", hr);
	hr = testCodePackage->CodePackageActivationContextCPtr->RegisterConfigurationPackageChangeHandler(configChangeHandlerCPtr.GetRawPointer(), &testCodePackage->ConfigHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterConfigurationPackageChangeHandler failed with {0}", hr);
	hr = testCodePackage->CodePackageActivationContextCPtr->RegisterDataPackageChangeHandler(dataChangeHandlerCPtr.GetRawPointer(), &testCodePackage->DataHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "RegisterDataPackageChangeHandler failed with {0}", hr);

    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "Registering servicetypes for CodePackage {0}", codePackageId);
    RegisterServiceTypes(testCodePackageContext.CodePackageId.CodePackageName, fabricRuntime, activationContext, testCodePackage->TestHostServiceFactory);

    codePackages_.insert(make_pair(codePackageId, testCodePackage));
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "SendCodePackageActivate {0}", codePackageId);
    SendCodePackageActivate(*serviceManifestSPtr, testCodePackage->TestCodePackageContext);
    TestSession::WriteNoise(TraceSource, FABRICHOSTSESSION.ClientId, "ActivateCodePackage {0} lock released", codePackageId);
}

void FabricTestHostDispatcher::DeactivateCodePackage(std::wstring const& codePackageId)
{
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "DeactivateCodePackage {0}", codePackageId);
    AcquireExclusiveLock grab(codePackagesLock_);
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "DeactivateCodePackage {0} acquiredLock", codePackageId);
    auto iter = codePackages_.find(codePackageId);
    TestSession::FailTestIf(iter == codePackages_.end(), "codePackageId {0} not found", codePackageId);

    HRESULT hr = iter->second->CodePackageActivationContextCPtr->UnregisterCodePackageChangeHandler(iter->second->CodeHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "UnregisterCodePackageChangeHandler failed with {0}", hr);
    hr = iter->second->CodePackageActivationContextCPtr->UnregisterConfigurationPackageChangeHandler(iter->second->ConfigHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "UnregisterConfigurationPackageChangeHandler failed with {0}", hr);
    hr = iter->second->CodePackageActivationContextCPtr->UnregisterDataPackageChangeHandler(iter->second->DataHandlerId);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "UnregisterDataPackageChangeHandler failed with {0}", hr);

    codePackages_.erase(codePackageId);
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "DeactivateCodePackage {0} releasing lock", codePackageId);
}

void FabricTestHostDispatcher::RegisterServiceTypes(
    wstring const& codePackageName, 
    ComPointer<IFabricRuntime> const& fabricRuntime,
    ComPointer<ComCodePackageActivationContext> const& activationContext, 
    TestHostServiceFactorySPtr const& testHostServiceFactory)
{
    shared_ptr<ServiceManifestDescription> serviceManifestDescription;
    auto error = activationContext->Test_CodePackageActivationContextObj.GetServiceManifestDescription(serviceManifestDescription);
    TestSession::FailTestIfNot(error.IsSuccess(), "GetServiceManifestDescription failed with {0}", error);

    vector<wstring> supportedServiceTypeNames;
    std::map<std::wstring, ConfigSettings> const& configSettingsMap = activationContext->Test_CodePackageActivationContextObj.Test_ConfigSettingsMap;
    TestSession::FailTestIf(configSettingsMap.size() != 1, "The application builder ensures 1 config package");
    auto iter = configSettingsMap.begin();
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigPackage....", codePackageName);
    ConfigSettings const& configSettings = iter->second;

    wstring codePackageRunAs;
    wstring defaultRunAs;
    wstring sharedPackage;

    for (auto iter1 = configSettings.Sections.begin(); iter1 != configSettings.Sections.end(); iter1++)
    {
        ConfigSection const& configSection = iter1->second;
        if(configSection.Name == FabricTestConstants::SupportedServiceTypesSection)
        {
            TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigSection {1}....", codePackageName, configSection.Name);

            for (auto iter2 = configSection.Parameters.begin(); iter2 != configSection.Parameters.end(); iter2++)
            {
                ConfigParameter const& configParameter = iter2->second;
                if(configParameter.Name == codePackageName)
                {
                    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigParam {1}:{2}....", codePackageName, configParameter.Name, configParameter.Value);
                    StringCollection serviceTypes;
                    wstring delimiter(L","); 
                    StringUtility::Split<wstring>(configParameter.Value, serviceTypes, delimiter);

                    for (auto iter3 = serviceTypes.begin(); iter3 != serviceTypes.end(); iter3++)
                    {
                        wstring const& serviceType = *iter3;
                        supportedServiceTypeNames.push_back(serviceType);
                    }
                }
            }
        }
        else if (configSection.Name == FabricTestConstants::CodePackageRunAsSection)
        {
            TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigSection {1}....", codePackageName, configSection.Name);

            for (auto iter2 = configSection.Parameters.begin(); iter2 != configSection.Parameters.end(); iter2++)
            {
                ConfigParameter const& configParameter = iter2->second;
                if(configParameter.Name == codePackageName)
                {
                    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigParam {1}....", codePackageName, configParameter.Name);
                    codePackageRunAs = configParameter.Value;
                }
            }
        }
        else if (configSection.Name == FabricTestConstants::DefaultRunAsSection)
        {
            TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigSection {1}....", codePackageName, configSection.Name);
            TestSession::FailTestIfNot(configSection.Parameters.size() == 1, "DefaultRunAsSection size is not 1");
            ConfigParameter const& configParameter = configSection.Parameters.begin()->second;
            defaultRunAs = configParameter.Value;
        }
        else if(configSection.Name == FabricTestConstants::PackageSharingSection)
        {
            TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found ConfigSection {1}....", codePackageName, configSection.Name);

            for (auto iter2 = configSection.Parameters.begin(); iter2 != configSection.Parameters.end(); iter2++)
            {
                ConfigParameter const& configParameter = iter2->second;
                if(configParameter.Name == codePackageName)
                {
                    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Found PackageSharing ConfigParam ....", configParameter.Name);
                    sharedPackage = configParameter.Name;
                }
            }
        }
    }

    if(!defaultRunAs.empty() || !codePackageRunAs.empty())
    {
        DWORD length = 0;
        GetUserNameW(NULL, &length);
        vector<WCHAR> nameBuffer(length);
        TestSession::FailTestIf(!GetUserNameW(nameBuffer.data(), &length), "GetUserNameW failed");
        wstring name(nameBuffer.data());
        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "DefaultRunAs '{0}' CodePackageRunAs '{1}' Actual UserName '{2}'", defaultRunAs, codePackageRunAs, name);

        wstring comment;
        error = SecurityPrincipalHelper::GetUserComment(name, comment);
        TestSession::FailTestIfNot(error.IsSuccess(), "SecurityPrincipalHelper::GetComment for name {0} failed with {1}", name, error);
        StringCollection collection;
        StringUtility::Split<wstring>(comment, collection, L"|");
        TestSession::FailTestIfNot(collection.size() == 4, "Invalid Comment {0}", comment);
        TestSession::FailTestIfNot(collection[0] == ApplicationPrincipals::WinFabAplicationGroupCommentPrefix, "Invalid Comment {0}", comment);
        StringCollection nodeIdCollection;
        StringUtility::Split<wstring>(collection[2], nodeIdCollection, L",");
        auto nodeIdIter = find(nodeIdCollection.begin(), nodeIdCollection.end(), FABRICHOSTSESSION.NodeId);
        TestSession::FailTestIf(nodeIdIter == nodeIdCollection.end(), "NodeId does not match Expected:{0} Actual:{1}", FABRICHOSTSESSION.NodeId, collection[2]);

        wstring originalManifestName = collection[1];
        if(!codePackageRunAs.empty())
        {
            TestSession::FailTestIfNot(
                StringUtility::CompareCaseInsensitive(originalManifestName, codePackageRunAs) == 0, 
                "RunAs comaprison failed with Name '{0}' and CodePackageRunAs '{1}'", 
                originalManifestName, 
                codePackageRunAs);
        }
        else
        {
            TestSession::FailTestIfNot(
                StringUtility::CompareCaseInsensitive(originalManifestName, defaultRunAs) == 0, 
                "RunAs comaprison failed with Name '{0}' and DefaultRunAs '{1}'", 
                originalManifestName, 
                defaultRunAs);
        }
    }
    if(!sharedPackage.empty())
    {
        //verify sharing policy
        VerifyPackageSharingPolicy(codePackageName, !sharedPackage.empty(), activationContext);
    }

    vector<ServiceTypeDescription> supportedServiceTypeDescriptions;
    for (auto serviceTypeNamesIter = serviceManifestDescription->ServiceTypeNames.begin(); serviceTypeNamesIter != serviceManifestDescription->ServiceTypeNames.end(); ++serviceTypeNamesIter)
    {
        auto servieTypeNameIter = find(supportedServiceTypeNames.begin(), supportedServiceTypeNames.end(), serviceTypeNamesIter->ServiceTypeName);
        if(servieTypeNameIter != supportedServiceTypeNames.end())
        {
            supportedServiceTypeDescriptions.push_back(*serviceTypeNamesIter);
        }
    }

    ComPointer<ComTestHostServiceFactory> comTestServiceFactoryCPtr = make_com<ComTestHostServiceFactory>(*testHostServiceFactory, supportedServiceTypeDescriptions);
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Created factory....", codePackageName);

    for (auto supportedServiceTypeDescIter = supportedServiceTypeDescriptions.begin(); supportedServiceTypeDescIter != supportedServiceTypeDescriptions.end(); supportedServiceTypeDescIter++)
    {
        wstring const& serviceType = supportedServiceTypeDescIter->ServiceTypeName;
        if(supportedServiceTypeDescIter->IsStateful)
        {
            HRESULT hr = fabricRuntime->RegisterStatefulServiceFactory(serviceType.c_str(), comTestServiceFactoryCPtr.GetRawPointer());
            TestSession::FailTestIf(FAILED(hr), "ComFabricRuntime::RegisterStatefulServiceFactory failed with {0} at client {1}:{2}", hr, codePackageName, FABRICHOSTSESSION.ClientId);
        }
        else
        {
            HRESULT hr = fabricRuntime->RegisterStatelessServiceFactory(serviceType.c_str(), comTestServiceFactoryCPtr.GetRawPointer());
            TestSession::FailTestIf(FAILED(hr), "ComFabricRuntime::RegisterStatelessServiceFactory failed with {0} at client {1}:{2}", hr, codePackageName, FABRICHOSTSESSION.ClientId);
        }

        TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "{0} Registered service type {1}....", codePackageName, serviceType);
    }
}

void FabricTestHostDispatcher::VerifyPackageSharingPolicy(
    wstring const& codePackageName, 
    bool isShared,
    ComPointer<ComCodePackageActivationContext> const& activationContext)
{
    TestSession::WriteInfo(TraceSource, FABRICHOSTSESSION.ClientId, "Verifying shared package policy for {0} and isShared {1}", codePackageName, isShared);
    IFabricCodePackage* fabricCodePackage;
    HRESULT error = activationContext->GetCodePackage(codePackageName.c_str(), & fabricCodePackage);
    if(error != S_OK)
    {
        TestSession::WriteError(TraceSource, FABRICHOSTSESSION.ClientId, "Failed to get CodePackage from activation context, codepackagename {0}, error {1}", codePackageName, error);
        TestSession::FailTest("Failed to get CodePackage from activation context, codepackagename {0}, error {1}", codePackageName, error);
    }
    DWORD fileAttributes = ::GetFileAttributes(fabricCodePackage->get_Path());
    wstring sharedFolderPattern(L".lk");
    if(isShared)
    {
        //shared folder must be a reparse point and end with .lk
        if(fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
            StringUtility::EndsWithCaseInsensitive((wstring)fabricCodePackage->get_Path(), sharedFolderPattern))
        {
            TestSession::WriteInfo(
                TraceSource, FABRICHOSTSESSION.ClientId,
                "Package {0} sharing via path {1} matches IsShared {2}",
                codePackageName,
                fabricCodePackage->get_Path(),
                isShared);
        }
        else
        {
            TestSession::FailTest(
                "Package {0} with path {1} does not match IsShared {2}",
                codePackageName,
                fabricCodePackage->get_Path(),
                isShared);
        }
    }
}


