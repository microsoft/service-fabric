// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

#define DLLEXPORT_(returnType) EXTERN_C returnType __stdcall

// {cbd93bc2-71e5-4566-b3a7-595d8eeca6e8}
static const GUID WinFabricProviderId = { 0xcbd93bc2, 0x71e5, 0x4566, { 0xb3, 0xa7, 0x59, 0x5d, 0x8e, 0xec, 0xa6, 0xe8 } };

// TODO: We should make this more elegant at some point
static wstring ExpectedFuidForReconfig;
static wstring ExpectedUpNodeId;

static map<USHORT, list<TracePointPayload>> eventListeners;
static const USHORT FMEventReceive = 18452;
static const USHORT RAEventReceive = 18692;

std::unique_ptr<TracePointTracer> traceUPtr;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(reserved);

    return TRUE;
}

void Crash()
{  
    HANDLE currentProcess = GetCurrentProcess();
    
    TerminateProcess(currentProcess, 0x1234); 

    WaitForSingleObject(currentProcess, INFINITE);
}

wstring GetActionFromController(wstring const & toServer, wstring const & fullyQualifiedPipeName)
{
    wstring log = L"GetActionFromController toServer=";
    log.append(toServer);
    log.append(L" fullyQualifiedPipeName=");
    log.append(fullyQualifiedPipeName);
    traceUPtr->Trace(log, TRACE_LEVEL_INFORMATION);
    TracePointPipeClient pipeClient(fullyQualifiedPipeName, *traceUPtr);
    pipeClient.WriteString(toServer);
    
    return pipeClient.ReadString();    
}

void ProcessRequest(wstring const& toServer, const USHORT eventReceived, wstring const& action)
{
    wstring log = L"ProcessRequest toServer=";
    log.append(toServer);
    log.append(L" action=");
    log.append(action);
    log.append(L" eventReceived=");
    traceUPtr->Trace(log, eventReceived, TRACE_LEVEL_INFORMATION);

    map<USHORT, list<TracePointPayload>>::iterator i = eventListeners.find(eventReceived);

    if(i != eventListeners.end())
    {
        list<TracePointPayload> payloads = (*i).second;

        for(list<TracePointPayload>::iterator payloadIterator = payloads.begin(); payloadIterator != payloads.end(); payloadIterator ++)
        {
            TracePointPayload tracePointPayload = (*payloadIterator);
            if(tracePointPayload.GetAction() == action || action == L"*")
            {
                wstring fromServer = GetActionFromController(toServer, tracePointPayload.GetFullyQualifiedPipeName());
                if(fromServer.find(L"CRASH") != wstring::npos)
                {       
                    Crash();
                }
            }
        }
    }
}

void OnFMEventReceived(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(EventDescriptor);

    if(UserDataCount == 4)
    {
        LPCWSTR fmid = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
        LPCWSTR action = reinterpret_cast<LPCWSTR>(UserData[1].Ptr);
        LPCWSTR fromNodeId = reinterpret_cast<LPCWSTR>(UserData[2].Ptr);
        UINT64 * fromInstanceId = reinterpret_cast<UINT64*>(UserData[3].Ptr);

        wstring fmidString(fmid);
        wstring nodeId = fmidString.substr(0, fmidString.find_first_of(L":"));

        std::wostringstream stringStream;

        stringStream << L"<FMEventReceive>";
        stringStream << L"<fmid>" << fmid << L"</fmid>";
        stringStream << L"<nodeId>" << nodeId << L"</nodeId>";
        stringStream << L"<action>" << action << L"</action>";
        stringStream << L"<fromNodeId>" << fromNodeId << L"</fromNodeId>";
        stringStream << L"<fromInstanceId>" << *fromInstanceId << L"</fromInstanceId>";
        stringStream << L"</FMEventReceive>";

        wstring toServer = stringStream.str();

        wstring log = L"OnFMEventReceived toServer=";
        log.append(toServer);
        traceUPtr->Trace(log, TRACE_LEVEL_INFORMATION);

        ProcessRequest(toServer, FMEventReceive, wstring(action));
    }
}

void OnRAEventReceived(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(EventDescriptor);

    if(UserDataCount == 5)
    {
        LPCWSTR nodeId = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
        UINT64 *instanceId = reinterpret_cast<UINT64*>(UserData[1].Ptr);
        LPCWSTR action = reinterpret_cast<LPCWSTR>(UserData[2].Ptr);
        LPCWSTR fromNodeId = reinterpret_cast<LPCWSTR>(UserData[3].Ptr);
        UINT64 * fromInstanceId = reinterpret_cast<UINT64*>(UserData[4].Ptr);
        std::wostringstream stringStream;

        stringStream << L"<RAEventReceive>";
        stringStream << L"<nodeId>" << nodeId << L"</nodeId>";
        stringStream << L"<instanceId>" << *instanceId << L"</instanceId>";
        stringStream << L"<action>" << action << L"</action>";
        stringStream << L"<fromNodeId>" << fromNodeId << L"</fromNodeId>";
        stringStream << L"<fromInstanceId>" << *fromInstanceId << L"</fromInstanceId>";
        stringStream << L"</RAEventReceive>";

        wstring toServer = stringStream.str();

        wstring log = L"OnRAEventReceived toServer=";
        log.append(toServer);
        traceUPtr->Trace(log, TRACE_LEVEL_INFORMATION);

        ProcessRequest(toServer, RAEventReceive, wstring(action));
    }
}

void OnNamingEventReceived(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
        std::wostringstream stringStream;

        if(UserDataCount == 4)
        {
            LPCWSTR actitivityId = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
            LPCWSTR nodeId = reinterpret_cast<LPCWSTR>(UserData[1].Ptr);
            LPCWSTR name = reinterpret_cast<LPCWSTR>(UserData[3].Ptr);

            stringStream << L"<NamingEventReceive>";
            stringStream << L"<actitivityId>" << actitivityId << L"</actitivityId>";
            stringStream << L"<fromNodeId>" << nodeId << L"</fromNodeId>";
            stringStream << L"<name>" << name << L"</name>";
            stringStream << L"<eventId>" << EventDescriptor->Id << L"</eventId>";
            stringStream << L"</NamingEventReceive>";
        }
        else if(UserDataCount == 6)
        {
            LPCWSTR actitivityId = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
            LPCWSTR authorityNodeId = reinterpret_cast<LPCWSTR>(UserData[1].Ptr);
            LPCWSTR name = reinterpret_cast<LPCWSTR>(UserData[3].Ptr);
            LPCWSTR nameNodeId = reinterpret_cast<LPCWSTR>(UserData[4].Ptr);

            stringStream << L"<NamingEventReceive>";
            stringStream << L"<actitivityId>" << actitivityId << L"</actitivityId>";
            stringStream << L"<fromNodeId>" << authorityNodeId << L"</fromNodeId>";
            stringStream << L"<toNodeId>" << nameNodeId << L"</toNodeId>";
            stringStream << L"<name>" << name << L"</name>";
            stringStream << L"<eventId>" << EventDescriptor->Id << L"</eventId>";
            stringStream << L"</NamingEventReceive>";  
        }

        wstring toServer = stringStream.str();
        ProcessRequest(toServer, EventDescriptor->Id, L"*");
}

void CrashOnReconfigStarted(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(EventDescriptor);

    if (UserDataCount == 1)
    {
        LPCWSTR fuid = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
    
        if (ExpectedFuidForReconfig == wstring(fuid))
        {
            Crash();
        }
    }
}

void CrashOnNodeAddedToGlobalFailoverUnitMap(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData)
{
    UNREFERENCED_PARAMETER(EventDescriptor);
    UNREFERENCED_PARAMETER(UserDataCount);
    
    LPCWSTR expectedNodeId = reinterpret_cast<LPCWSTR>(UserData[0].Ptr);
       
    if (ExpectedUpNodeId == wstring(expectedNodeId))
    {       
        Crash();
    }       
}

DLLEXPORT_(void) Setup(ProviderMapHandle h, LPCWSTR userData)
{
    UNREFERENCED_PARAMETER(userData);

    traceUPtr = make_unique<TracePointTracer>(h);
}

DLLEXPORT_(void) EnableCallMe(ProviderMapHandle h, LPCWSTR userData)
{
    UNREFERENCED_PARAMETER(h);
        
    TracePointPayload tracePointPayload;
    tracePointPayload.Parse(wstring(userData));

    switch(tracePointPayload.GetMessageId())
    {
        case FMEventReceive:
            if(eventListeners.find(tracePointPayload.GetMessageId()) == eventListeners.end())
            {
                TracePointsAddEventMapEntry(h, WinFabricProviderId, FMEventReceive, OnFMEventReceived, nullptr);
            }

            eventListeners[tracePointPayload.GetMessageId()].push_back(tracePointPayload);
            break;
        case RAEventReceive:
            if(eventListeners.find(tracePointPayload.GetMessageId()) == eventListeners.end())
            {
                TracePointsAddEventMapEntry(h, WinFabricProviderId, RAEventReceive, OnRAEventReceived, nullptr);
            }

            eventListeners[tracePointPayload.GetMessageId()].push_back(tracePointPayload);
            break; 
        case WinFabricStructuredTracingCodes::AuthorityOwnerInnerCreateNameReplyReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerCreateNameRequestReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerCreateNameRequestProcessComplete:
        case WinFabricStructuredTracingCodes::AuthorityOwnerInnerDeleteNameReplyReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerDeleteNameRequestReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerDeleteNameRequestProcessComplete:
        case WinFabricStructuredTracingCodes::AuthorityOwnerInnerCreateServiceReplyFromNameOwnerReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerCreateServiceReplyFromFMReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerCreateServiceRequestReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerCreateServiceRequestProcessComplete:
        case WinFabricStructuredTracingCodes::AuthorityOwnerInnerDeleteServiceReplyFromNameOwnerReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerDeleteServiceReplyFromFMReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerDeleteServiceRequestReceiveComplete:
        case WinFabricStructuredTracingCodes::NameOwnerInnerDeleteServiceRequestProcessComplete:
        case WinFabricStructuredTracingCodes::StartupRepairCompleted:
        case WinFabricStructuredTracingCodes::AuthorityOwnerHierarchyRepairCompleted:
        case WinFabricStructuredTracingCodes::AuthorityOwnerServiceRepairCompleted:
            if(eventListeners.find(tracePointPayload.GetMessageId()) == eventListeners.end())
            {
                TracePointsAddEventMapEntry(h, WinFabricProviderId, tracePointPayload.GetMessageId(), OnNamingEventReceived, nullptr);
            }

            eventListeners[tracePointPayload.GetMessageId()].push_back(tracePointPayload);
            break;
        default:
            break;
    }
}

DLLEXPORT_(void) EnableCrashOnNodeAddedToGFUM(ProviderMapHandle h, LPCWSTR userData)
{
    ExpectedUpNodeId = wstring(userData);   
    TracePointsAddEventMapEntry(h, WinFabricProviderId, WinFabricStructuredTracingCodes::FMM_NodeAddedToGFUM, CrashOnNodeAddedToGlobalFailoverUnitMap, nullptr);
}

DLLEXPORT_(void) Cleanup(ProviderMapHandle h, LPCWSTR userData)
{
    UNREFERENCED_PARAMETER(h);
    
    TracePointPayload tracePointPayload;
    tracePointPayload.Parse(wstring(userData));

    TracePointsRemoveEventMapEntry(h, WinFabricProviderId, tracePointPayload.GetMessageId());
    eventListeners.erase(tracePointPayload.GetMessageId());
}
