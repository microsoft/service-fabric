// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

GlobalWString TestPortHelper::EnvVarName_WinFabTestPorts = make_global<std::wstring>(L"WinFabTestPorts");
USHORT const TestPortHelper::DefaultStartPort = 22000;
USHORT const TestPortHelper::DefaultEndPort = 22979;
USHORT const TestPortHelper::DefaultEndLeasePort = 22999;

USHORT TestPortHelper::startPort_ = 0;
USHORT TestPortHelper::endPort_ = 0;
USHORT TestPortHelper::endLeasePort_ = 0;
USHORT TestPortHelper::currentIndex_ = 0;
USHORT TestPortHelper::leaseIndex_ = 0;

ExclusiveLock TestPortHelper::portLock_;

StringLiteral const TraceComponent("TestPortHelper");
WStringLiteral const TraceTypeGetPorts(L"GetPorts");
WStringLiteral const TraceGetPortOwnerInfo(L"GetPortOwnerInfo");

static const int GetTcpTableRetryLimit = 3;

USHORT TestPortHelper::GetNextLeasePort()
{
    USHORT nextPort = endPort_ + 1 + leaseIndex_;
    if (nextPort == endLeasePort_)
    {
        leaseIndex_ = 0;
    }
    else
    {
        ++leaseIndex_;
    }

    return nextPort;
}

void TestPortHelper::GetPorts(USHORT numberOfPorts, USHORT & startPort)
{
    AcquireExclusiveLock grab(portLock_);
    if(startPort_ == 0 && endPort_ == 0)
    {
        std::wstring ports;
        if(Environment::GetEnvironmentVariableW(TestPortHelper::EnvVarName_WinFabTestPorts, ports, NOTHROW()))
        {
            StringCollection portsCollection;
            StringUtility::Split<std::wstring>(ports, portsCollection, L",");
            if(portsCollection.size() == 2)
            {
                startPort_ = static_cast<USHORT>(Int32_Parse(portsCollection[0]));
                endLeasePort_ = static_cast<USHORT>(Int32_Parse(portsCollection[1]));
                ASSERT_IF(endLeasePort_ - startPort_ < 500, "Not enough ports supplied to TestPortHelpers");
                endPort_ = endLeasePort_ - 20;
            }
            else
            {
                Trace.WriteWarning(
                    TraceComponent,
                    wstring(TraceTypeGetPorts.begin()),
                    "Invalid environment {0}. Reverting to default",
                    ports);
                startPort_ = TestPortHelper::DefaultStartPort;
                endPort_ = TestPortHelper::DefaultEndPort;
                endLeasePort_ = TestPortHelper::DefaultEndLeasePort;
            }
        }
        else
        {
            Trace.WriteNoise(
                TraceComponent,
                wstring(TraceTypeGetPorts.begin()),
                "Env {0} not set. Reverting to default",
                TestPortHelper::EnvVarName_WinFabTestPorts);
            startPort_ = TestPortHelper::DefaultStartPort;
            endPort_ = TestPortHelper::DefaultEndPort;
            endLeasePort_ = TestPortHelper::DefaultEndLeasePort;
        }

        currentIndex_ = startPort_;
    }

    ASSERT_IF(numberOfPorts > (endPort_ - startPort_) + 1, "Cannot assign {0} ports at TestPortHelper", numberOfPorts);

    if((currentIndex_ + numberOfPorts - 1) <= endPort_)
    {
        Trace.WriteInfo(
            TraceComponent,
            wstring(TraceTypeGetPorts.begin()),
            "Allocating test ports from [{0}, {1}]: start = {2} count = {3}",
            startPort_,
            endPort_,
            currentIndex_,
            numberOfPorts);

        startPort = currentIndex_;
        currentIndex_  = currentIndex_ + numberOfPorts;
    }
    else
    {
        Trace.WriteInfo(
            TraceComponent,
            wstring(TraceTypeGetPorts.begin()),
            "Allocating overflow test ports from [{0}, {1}]: count = {2}",
            startPort_,
            endPort_,
            numberOfPorts);

        startPort = startPort_;
        currentIndex_ = startPort_ + numberOfPorts;
    }
}

#if !defined(PLATFORM_UNIX)
template <typename TTcpTable, typename TTcpRow>
ErrorCode GetTcpPortOwnerInfoImpl(
    ULONG (*TcpTableFunc) (TTcpTable* tcpTable, PULONG sizePointer, BOOL order),
    USHORT port,
    wstring _Out_ & ownerInfo)
{
    ownerInfo.clear();

    ULONG buffSize = 0;
    DWORD getSizeErr = TcpTableFunc(nullptr, &buffSize, FALSE);
    if (getSizeErr != ERROR_INSUFFICIENT_BUFFER)
    {
        Trace.WriteWarning(
            TraceComponent,
            TraceGetPortOwnerInfo.begin(),
            "GetTcpTable() for buffer size failed with {0}",
            getSizeErr);

        return ErrorCode::FromWin32Error(getSizeErr);
    }

    vector<byte> buffer;
    for(int i = 0; ; ++i)
    {
        buffer.resize(buffSize);
        DWORD getTableErr = TcpTableFunc(reinterpret_cast<TTcpTable*>(buffer.data()), &buffSize, FALSE);
        if (getTableErr == NO_ERROR) break;

        if ((getTableErr == ERROR_INSUFFICIENT_BUFFER) && (i < GetTcpTableRetryLimit)) continue;

        Trace.WriteWarning(
            TraceComponent,
            TraceGetPortOwnerInfo.begin(),
            "GetTcpTable() failed with {0}",
            getSizeErr);

        return ErrorCode::FromWin32Error(getSizeErr);
    }

    TTcpTable const & socketTable = reinterpret_cast<TTcpTable&>(buffer.front());
    for (DWORD ix = 0; ix < socketTable.dwNumEntries; ++ix)
    {
        TTcpRow const & tcpRow = socketTable.table[ix];
        USHORT localPort = ntohs(static_cast<USHORT>(tcpRow.dwLocalPort));
        if (localPort == port)
        {
            ownerInfo = wformatString("process {0}", tcpRow.dwOwningPid);
            if (tcpRow.dwOwningPid != 0) // 0 means socket is in wait states
            {
                wstring processImageName;
                if (ProcessUtility::GetProcessImageName(tcpRow.dwOwningPid, processImageName).IsSuccess())
                {
                    ownerInfo += L' ';
                    ownerInfo += std::move(processImageName);
                }
            }

            return ErrorCode();
        }
    }

    return ErrorCodeValue::OperationFailed;
}
#endif

template <typename TTcpTable, typename TTcpRow>
TestPortHelper::PortMap GetPortsInUseImpl(ULONG(*TcpTableFunc) (TTcpTable* tcpTable, PULONG sizePointer, BOOL order))
{
    TestPortHelper::PortMap portsInUse;

    ULONG buffSize = 0;
    DWORD getSizeErr = TcpTableFunc(nullptr, &buffSize, FALSE);
    if (getSizeErr != ERROR_INSUFFICIENT_BUFFER)
    {
        Trace.WriteWarning(
            TraceComponent,
            TraceGetPortOwnerInfo.begin(),
            "GetTcpTable() for buffer size failed with {0}",
            getSizeErr);

        return portsInUse; 
    }

    vector<byte> buffer;
    for(int i = 0; ; ++i)
    {
        buffer.resize(buffSize);
        DWORD getTableErr = TcpTableFunc(reinterpret_cast<TTcpTable*>(buffer.data()), &buffSize, FALSE);
        if (getTableErr == NO_ERROR) break;

        if ((getTableErr == ERROR_INSUFFICIENT_BUFFER) && (i < GetTcpTableRetryLimit)) continue;

        Trace.WriteWarning(
            TraceComponent,
            TraceGetPortOwnerInfo.begin(),
            "GetTcpTable() failed with {0}",
            getSizeErr);

        return portsInUse; 
    }

    TTcpTable const & socketTable = reinterpret_cast<TTcpTable&>(buffer.front());
    for (DWORD ix = 0; ix < socketTable.dwNumEntries; ++ix)
    {
        TTcpRow const & tcpRow = socketTable.table[ix];
        USHORT localPort = ntohs(static_cast<USHORT>(tcpRow.dwLocalPort));
        portsInUse.insert(make_pair(localPort, tcpRow.dwOwningPid));
    }

    return portsInUse;
}
 
#ifdef PLATFORM_UNIX

#include <fstream>

//
// copied from iphlpapi.h {
//

#define ANY_SIZE 1

typedef enum {
    MIB_TCP_STATE_CLOSED     =  1,
    MIB_TCP_STATE_LISTEN     =  2,
    MIB_TCP_STATE_SYN_SENT   =  3,
    MIB_TCP_STATE_SYN_RCVD   =  4,
    MIB_TCP_STATE_ESTAB      =  5,
    MIB_TCP_STATE_FIN_WAIT1  =  6,
    MIB_TCP_STATE_FIN_WAIT2  =  7,
    MIB_TCP_STATE_CLOSE_WAIT =  8,
    MIB_TCP_STATE_CLOSING    =  9,
    MIB_TCP_STATE_LAST_ACK   = 10,
    MIB_TCP_STATE_TIME_WAIT  = 11,
    MIB_TCP_STATE_DELETE_TCB = 12,
} MIB_TCP_STATE;

typedef enum  { 
  TcpConnectionOffloadStateInHost      = 0,
  TcpConnectionOffloadStateOffloading  = 1,
  TcpConnectionOffloadStateOffloaded   = 2,
  TcpConnectionOffloadStateUploading   = 3,
  TcpConnectionOffloadStateMax         = 4
} TCP_CONNECTION_OFFLOAD_STATE;

typedef struct _MIB_TCPROW2 {
  DWORD                        dwState;
  DWORD                        dwLocalAddr;
  DWORD                        dwLocalPort;
  DWORD                        dwRemoteAddr;
  DWORD                        dwRemotePort;
  DWORD                        dwOwningPid;
  TCP_CONNECTION_OFFLOAD_STATE dwOffloadState;
} MIB_TCPROW2, *PMIB_TCPROW2;

typedef struct _MIB_TCPTABLE2 {
  DWORD       dwNumEntries;
  MIB_TCPROW2 table[ANY_SIZE];
} MIB_TCPTABLE2, *PMIB_TCPTABLE2;

typedef struct _MIB_TCP6ROW2 {
  IN6_ADDR                     LocalAddr;
  DWORD                        dwLocalScopeId;
  DWORD                        dwLocalPort;
  IN6_ADDR                     RemoteAddr;
  DWORD                        dwRemoteScopeId;
  DWORD                        dwRemotePort;
  MIB_TCP_STATE                State;
  DWORD                        dwOwningPid;
  TCP_CONNECTION_OFFLOAD_STATE dwOffloadState;
} MIB_TCP6ROW2, *PMIB_TCP6ROW2;

typedef struct _MIB_TCP6TABLE2 {
  DWORD        dwNumEntries;
  MIB_TCP6ROW2 table[ANY_SIZE];
} MIB_TCP6TABLE2, *PMIB_TCP6TABLE2;

// } iphlpapi.h

static vector<string> ReadProcLines(string const & path)
{
    ifstream procFile(path, ifstream::in);
    vector<string> lines;

    // ignore the header line
    string line;
    if (!std::getline(procFile, line))
    {
        return lines;
    }

    while (std::getline(procFile, line))
    {
        StringUtility::TrimWhitespaces(line);
        if (line.empty()) continue;

        lines.emplace_back(move(line));
    }

    return lines;
}

template <typename TTcpTable, typename TTcpRow>
ULONG WINAPI GetTcpTableImpl(
    _In_ string const & procFile,
    _Out_ TTcpTable * TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order)
{
    Invariant(Order == FALSE);

    if (!SizePointer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    auto lines = ReadProcLines(procFile);
    if (lines.empty())
    {
        return NO_ERROR;
    }

    size_t sizeNeeded = sizeof(TTcpTable) + (lines.size() - 1) * sizeof(TTcpRow);
    if (*SizePointer < sizeNeeded)
    {
        // Request space larger than needed in case the number of rows grow for next call
        *SizePointer = sizeNeeded * 2;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    TcpTable->dwNumEntries = 0;
    *SizePointer = sizeof(TTcpTable) - sizeof(TTcpRow);
    for(auto const & line : lines)
    {
        vector<string> tokens;
        StringUtility::Split<string>(line, tokens, " :");
        if (tokens.size() < 3) continue;

        USHORT port = 0;
        stringstream(tokens[2]) >> hex >> port; 
        TcpTable->table[TcpTable->dwNumEntries] = { .dwLocalPort = htons(port) };
        ++ TcpTable->dwNumEntries;
        *SizePointer += sizeof(TTcpRow);
    }

    return NO_ERROR;
}

//LINUXTODO consider moving the following two APIs to pal_missing.cpp
ULONG WINAPI GetTcpTable2(
_Out_ PMIB_TCPTABLE2 TcpTable,
_Inout_ PULONG SizePointer,
_In_ BOOL Order)
{
    return GetTcpTableImpl<MIB_TCPTABLE2, MIB_TCPROW2>("/proc/net/tcp", TcpTable, SizePointer, Order);
}

ULONG WINAPI GetTcp6Table2(
_Out_ PMIB_TCP6TABLE2 TcpTable,
_Inout_ PULONG SizePointer,
_In_ BOOL Order)
{
    return GetTcpTableImpl<MIB_TCP6TABLE2, MIB_TCP6ROW2>("/proc/net/tcp6", TcpTable, SizePointer, Order);
}

string TestPortHelper::GetNetStatTcp()
{
    string output;
    ProcessUtility::GetStatusOutput("netstat -a -t -p", output);
    return output;
}

string TestPortHelper::GetTcpPortOwnerInfo(USHORT port)
{
    string owner;
    GetTcpPortOwnerInfo(port, owner);
    return owner;
}

_Use_decl_annotations_ ErrorCode TestPortHelper::GetTcpPortOwnerInfo(USHORT port, string & ownerInfo)
{
    ownerInfo.clear();

    vector<string> lines;
    string cmd = formatString("netstat -t -l -p | grep :{0}", port);
    auto error = ProcessUtility::GetStatusOutput(cmd, lines);

    for(auto const & line : lines)
    {
        vector<string> tokens;
        StringUtility::Split<string>(line, tokens, " \n");

        if (tokens.size() <= 2) continue;
        if (!StringUtility::AreEqualCaseInsensitive(tokens[tokens.size() - 2], "LISTEN")) continue;

        ownerInfo = tokens[tokens.size() - 1];
        break;
    }

    return error;
}

#else

_Use_decl_annotations_ ErrorCode TestPortHelper::GetTcpPortOwnerInfo(USHORT port, wstring & ownerInfo)
{
    return GetTcpPortOwnerInfoImpl<MIB_TCPTABLE2, MIB_TCPROW2>(::GetTcpTable2, port, ownerInfo);
}

_Use_decl_annotations_ ErrorCode TestPortHelper::GetTcp6PortOwnerInfo(USHORT port, wstring & ownerInfo)
{
    return GetTcpPortOwnerInfoImpl<MIB_TCP6TABLE2, MIB_TCP6ROW2>(::GetTcp6Table2, port, ownerInfo);
}

#endif

TestPortHelper::PortMap TestPortHelper::GetPortsInUse()
{
    auto portsInUse = GetPortsInUseImpl<MIB_TCPTABLE2, MIB_TCPROW2>(::GetTcpTable2);
    auto portsInUseIpV6 = GetPortsInUseImpl<MIB_TCP6TABLE2, MIB_TCP6ROW2>(::GetTcp6Table2);
    portsInUse.insert(portsInUseIpV6.begin(), portsInUseIpV6.end()); 
    return portsInUse;
}
