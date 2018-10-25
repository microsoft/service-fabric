// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace FederationTestCommon;
using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Transport;

StringLiteral const TraceSource("FabricTestHost");
wstring Label = L"FabricTestHost";
wstring CodePackageName = L"";

int main(int argc, __in_ecount( argc ) char** argv) 
{
#ifndef PLATFORM_UNIX 
    CRTAbortBehavior::DisableAbortPopup();    
#endif

    { Config config; } // trigger tracing config loading
    TraceTextFileSink::SetPath(L""); // Empty trace file path so that nothing is traced until the actual path is set

    //
    // Enable KTL tracing into the SF file log
    //
    RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);
		
    StringCollection args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(wstring(argv[i], argv[i] + strlen(argv[i])));
    }

    auto pid = GetCurrentProcessId();

    CommandLineParser parser(args);

    auto isGuestExe = parser.GetBool(L"isGuestExe");
    if (isGuestExe)
    {
        TestSession::WriteInfo(TraceSource, "Fabric test Host started as guest exe with process id {0}.", pid);

        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
        return 0;
    }

    EnvironmentMap envMap;
    TestSession::FailTestIfNot(Environment::GetEnvironmentMap(envMap), "GetEnvironmentMap failed");

    wstring serverPort;
    TestSession::FailTestIfNot(parser.TryGetString(L"serverport", serverPort), "failed to parse serverport");
    AddressHelper::ServerListenPort = static_cast<USHORT>(Int32_Parse(serverPort));

    wstring testDir;
    TestSession::FailTestIfNot(parser.TryGetString(L"testdir", testDir), "failed to parse testdir");

    bool useEtw = parser.GetBool(L"useetw");

    wstring clientCredentialType;
    bool parseSecurityResult = parser.TryGetString(L"security", clientCredentialType);
    if (!parseSecurityResult)
    {
        TestSession::WriteWarning(TraceSource, "Could not parse security parameter: args={0}", args);

        clientCredentialType = L"None";
    }

    Transport::SecurityProvider::Enum securityProvider;
    auto error = Transport::SecurityProvider::FromCredentialType(clientCredentialType, securityProvider);
    TestSession::FailTestIfNot(error.IsSuccess(), "SecurityProvider::FromCredentialType failed with {0}: type={1}", error, clientCredentialType);

    ApplicationHostContext hostContext;
    error = ApplicationHostContext::FromEnvironmentMap(envMap, hostContext);
    TestSession::FailTestIfNot(error.IsSuccess(), "ApplicationHostContext::FromEnvironmentMap failed with {0}", error);

    if(useEtw)
    {
        TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Noise);
        TraceProvider::GetSingleton()->AddFilter(TraceSinkType::ETW, L"EseStore:4");
        TraceProvider::GetSingleton()->AddFilter(TraceSinkType::ETW, L"Transport:4");
    }

    wstring traceFilePath = Path::Combine(testDir, Path::Combine(FabricTestConstants::TestTracesDirectory, FabricTestConstants::TestHostTracesDirectory));    

    wstring nodeId;
    if(hostContext.HostType == ApplicationHostType::Activated_MultiCodePackage)
    {
        // TODO: SQL Server: Defect 1461864: FabricTestHost should register IFabricCodePackageHost first to get NodeId
        auto iter = envMap.find(Hosting2::Constants::EnvironmentVariable::NodeId);
        TestSession::FailTestIf(iter == envMap.end(), "NodeId should be in the environment", error);

        nodeId = iter->second;

        if(!useEtw)
        {
            wstring traceFileName = L"FabricTestMultiHost-" + nodeId + L"-" + hostContext.HostId + L".trace";
            TraceTextFileSink::SetPath(Path::Combine(traceFilePath, traceFileName));
        }

        TestMultiPackageHostContext testMultiPackageHostContext(nodeId, hostContext.HostId);
        wstring label = hostContext.HostId + nodeId;
        FabricTestHostSession::CreateSingleton(nodeId, label, testMultiPackageHostContext.ToClientId(), true);
        FABRICHOSTSESSION.Dispatcher.SetAsMultiPackageHost(testMultiPackageHostContext);
    }
    else if (hostContext.HostType == ApplicationHostType::Activated_SingleCodePackage)
    {
        ComPointer<ComFabricNodeContextResult> comFabricNodeContextCPtr;
        error = ErrorCode::FromHResult(ApplicationHostContainer::FabricGetNodeContext(comFabricNodeContextCPtr.VoidInitializationAddress()));
        TestSession::FailTestIfNot(error.IsSuccess(), "GetComFabricGetNodeContext failed with {0}", error);

        FABRIC_NODE_CONTEXT const * fabricNodeContext = comFabricNodeContextCPtr->get_NodeContext();       
        
        if(fabricNodeContext->NodeId.High == 0)
        {
			nodeId = wformatString("{0:x}", fabricNodeContext->NodeId.Low);
        }
        else
        {
			nodeId = wformatString("{0:x}{1:016x}", fabricNodeContext->NodeId.High, fabricNodeContext->NodeId.Low);
        }        

        wstring instanceId = wformatString(DateTime::Now().Ticks);
        if(!useEtw)
        {
            wstring traceFileName = L"FabricTestHost-" + nodeId + L"-" + instanceId + L".trace";
            TraceTextFileSink::SetPath(Path::Combine(traceFilePath, traceFileName));
            TraceTextFileSink::SetOption(L"p");
        }

        wstring version;
        TestSession::FailTestIfNot(parser.TryGetString(L"version", version), "failed to parse version");

        ComPointer<ComCodePackageActivationContext> comCodePackageActivationContextCPtr; 
        error = ErrorCode::FromHResult(ApplicationHostContainer::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, comCodePackageActivationContextCPtr.VoidInitializationAddress()));
        TestSession::FailTestIfNot(error.IsSuccess(), "GetComCodePackageActivationContext failed with {0}", error);
        TestCodePackageContext testCodePackageContext(nodeId, comCodePackageActivationContextCPtr->Test_CodePackageActivationContextObj.CodePackageInstanceId, instanceId, version);
        wstring label = wformatString("{0}-{1}-{2}-{3}-{4}", 
            testCodePackageContext.CodePackageId.ServicePackageInstanceId.ApplicationId,
            testCodePackageContext.CodePackageId.ServicePackageInstanceId.ServicePackageName,
            testCodePackageContext.CodePackageId.CodePackageName,
            nodeId,
            instanceId);
        FabricTestHostSession::CreateSingleton(nodeId, label, testCodePackageContext.ToClientId(), true);
        FABRICHOSTSESSION.Dispatcher.SetAsSinglePackageHost(comCodePackageActivationContextCPtr, testCodePackageContext);
    }
    else
    {
        TestSession::FailTest("Unknown HostType {0}", hostContext.HostType);
    }

    TestSession::FailTestIf(nodeId.empty(), "NodeId cannot be empty");

    if (securityProvider != SecurityProvider::None)
    {
        error = FABRICHOSTSESSION.Dispatcher.InitializeClientSecuritySettings(securityProvider);

        TestSession::FailTestIf(
            !error.IsSuccess(), 
            "InitializeClientSecuritySettings failed: error={0} ({1})",
            error,
            error.Message);
    }

    DateTime startTime = DateTime::Now();
    TestSession::WriteInfo(TraceSource, "Fabric test Host started with process id {0} and NodeId {1}....", pid, nodeId);
    int result = FABRICHOSTSESSION.Execute();
    TestSession::WriteInfo(TraceSource,"Fabric host session exited with result {0}, time spent: {1} ", result, DateTime::Now() - startTime);

    return result;
}
