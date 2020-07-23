// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestDispatcher.h"
#include "TestSession.h"
#include "RandomTestSession.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;
using namespace LBSimulator;

wstring Label = L"LBSimulator";
static wstring LoadFile = L"";
static wstring TestDataFile = L"";
static wstring RandDataFile = L"";
static wstring PlacementDumpFile = L"";
static wstring Mode = L"";
bool Auto = false;
bool RandomSession = false;
int Iterations = 0;
int Seed = 54321;

void OutputUsage()
{
    RealConsole().WriteLine("WinFabric placement and load balancing simulator");
    RealConsole().WriteLine("");
    RealConsole().WriteLine("LBSIMULATOR -r iterations");
    RealConsole().WriteLine("or");
    RealConsole().WriteLine("LBSIMULATOR [-auto] -load filename");
    RealConsole().WriteLine("or");
    RealConsole().WriteLine("LBSIMULATOR [-auto] -testdata filename");
    RealConsole().WriteLine("or");
    RealConsole().WriteLine("LBSIMULATOR [-auto] -randdata filename -seed seed");
    RealConsole().WriteLine("or");
    RealConsole().WriteLine("LBSIMULATOR [-auto] -placementdump filename -mode mode");
}

bool ParseArguments(StringCollection const& args)
{
    for(size_t i = 1; i < args.size(); ++i)
    {
        if (args[i].empty())
        {
            continue;
        }
        else if (args[i] == L"-load") 
        {
            LoadFile = args[++i];
        }
        else if (args[i] == L"-auto") 
        {
            Auto = true;
        }
        else if (args[i] == L"-testdata")
        {
            TestDataFile = args[++i];
        }
        else if (args[i] == L"-randdata")
        {
            RandDataFile = args[++i];
        }
        else if (args[i] == L"-seed")
        {
            Seed = _wtoi(args[++i].c_str());
        }
        else if (args[i] == L"-placementdump")
        {
            PlacementDumpFile = args[++i];
        }
        else if (args[i] == L"-mode")
        {
            Mode = args[++i];
        }
        else if (args[i] == L"-r") 
        {
            RandomSession = true;
            if(args.size() > i + 1)
            {
                Iterations = _wtoi(args[++i].c_str());
                TESTASSERT_IF(Iterations <= 0, "Iteration number should be larger than 0");
            }
            else
            {
                Iterations = std::numeric_limits<int>::max();
            }
        }
        else if (i == args.size() - 1 && LoadFile.size() == 0)
        {
            LoadFile = args[i];
        }
        else
        {
            RealConsole().WriteLine("Unknown command line option {0}", args[i]);
            OutputUsage();
            return false;
        }
    }

    return true;
}

int __cdecl wmain(int argc, __in_ecount( argc ) wchar_t ** argv) 
{

    { Common::Config config; } // trigger tracing config loading
    TraceTextFileSink::SetPath(L""); // Empty trace file path so that nothing is traced until the actual path is set

    Common::StringCollection args(argv+0, argv+argc);
    if (!ParseArguments(args))
    {
        return -1;
    }

    if(RandomSession)
    {
        TraceTextFileSink::SetOption(L"h");
    }

    TestDispatcher testDispatcher;
    shared_ptr<TestSession> session;

    Reliability::LoadBalancingComponent::PLBConfig & config = testDispatcher.PLBConfigObj;

    if(RandomSession)
    {
        Label = L"LBSimulationRandom";
        shared_ptr<RandomTestSession> randomSession = make_shared<RandomTestSession>(Iterations, Label, Auto, testDispatcher);
        session = static_pointer_cast<TestSession>(randomSession);

        wstring traceFileName = Label + L".trace";
        TraceTextFileSink::SetPath(traceFileName);
    }
    else
    {
        if (!LoadFile.empty())
        {
            Label = Path::GetFileName(LoadFile);
        }
        else if (!TestDataFile.empty())
        {
            Label = Path::GetFileName(TestDataFile);
        }
        else if (!RandDataFile.empty())
        {
            Label = Path::GetFileName(RandDataFile);
        }
        else if (!PlacementDumpFile.empty())
        {
            Label = Path::GetFileName(PlacementDumpFile);
        }

        session = make_shared<TestSession>(Label, Auto, testDispatcher);
        wstring traceFileName = Label + L".trace";

        if(!LoadFile.empty())
        {
            session->AddCommand(wstring(L"!load,") + LoadFile);
        }
        else if (!PlacementDumpFile.empty())
        {
            session->AddCommand(wstring(L"loadplacementdump ") + PlacementDumpFile);

            if (StringUtility::AreEqualCaseInsensitive(Mode.c_str(), L"creation"))
            {
                session->AddCommand(L"executeplacementdump creation");
            }
            else if (StringUtility::AreEqualCaseInsensitive(Mode.c_str(), L"constraintcheck"))
            {
                session->AddCommand(L"executeplacementdump constraintcheck");
            }
            else if (StringUtility::AreEqualCaseInsensitive(Mode.c_str(), L"loadbalancing"))
            {
                session->AddCommand(L"executeplacementdump loadbalancing");
            }
            else
            {
                Assert::CodingError("Invalid mode {0}", Mode);
            }

            if (Auto)
            {
                session->AddCommand(L"!q");
            }
        }
        else if (!TestDataFile.empty() || !RandDataFile.empty())
        {
            wstring loadCommand;
            
            if (!TestDataFile.empty())
            {
                loadCommand = wstring(L"loadplacement ") + TestDataFile;
            }
            else
            {
                loadCommand = wstring(L"loadrandomplacement ") + RandDataFile + wstring(L" ") + StringUtility::ToWString(Seed);;
            }

            session->AddCommand(loadCommand);
            session->AddCommand(L"executeplb");
            session->AddCommand(L"executeplb");

            if (Auto)
            {
                session->AddCommand(L"!q");
            }

            config.PLBRefreshInterval = Common::TimeSpan::MaxValue;
            config.MinLoadBalancingInterval = Common::TimeSpan::Zero;
            config.PlacementSearchTimeout = Common::TimeSpan::MaxValue;
            config.FastBalancingSearchTimeout = Common::TimeSpan::MaxValue;
            config.SlowBalancingSearchTimeout = Common::TimeSpan::MaxValue;
            config.ProcessPendingUpdatesInterval = Common::TimeSpan::MaxValue;
            config.YieldDurationPer10ms = 0;
            config.InitialRandomSeed = 12345;
            config.MaxSimulatedAnnealingIterations = -1;
            config.MaxPercentageToMove = 0.75;

            config.LoadBalancingEnabled = true;
            config.IgnoreCostInScoring = false;
        }

        TraceTextFileSink::SetPath(traceFileName);

    }

    DateTime startTime = DateTime::Now();
    DWORD pid = GetCurrentProcessId();
    TestSession::WriteInfo("LBSimulator", "LBSimulator started with process id {0}....", pid);
    int result = session->Execute();
    TestSession::WriteInfo("LBSimulator",
        "LBSimulator session exited with result {0}, time spent: {1} ",
        result, DateTime::Now() - startTime);

    return result;
}
