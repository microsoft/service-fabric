// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define __null 0
#undef _WIN64
#undef WIN32_LEAN_AND_MEAN
#include <grpc++/grpc++.h>
#include "cri-api.grpc.pb.h"
#define _WIN64 1
#define WIN32_LEAN_AND_MEAN 1
#include "stdafx.h"
#include "ContainerRuntimeHandler.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Hosting2;
using namespace Common;
using namespace ServiceModel;

StringLiteral TraceType_ContainerRuntimeHandlerTest = "ContainerRuntimeHandlerTest";

namespace Hosting2
{
    BOOST_AUTO_TEST_SUITE(TestContainerRuntimeHandler)

    BOOST_AUTO_TEST_CASE(BasicRunPodSandBox)
    {
#if defined(CLEAR_CONTAINER_INSTALLED)
        vector<wstring> sandboxNameVec;
        map<wstring, vector<wstring>> containerNameMap;
        wstring apphostid = L"AppHostId";
        wstring nodeid = L"NodeId";
        wstring traceid = L"BasicRunPodSandBox";
        //ManualResetEvent waiter;
        const int PodCnt = 1;
        const int ContainerCntPerPod = 1;

        atomic_int cnt(0);

        // Create Pods
        vector<Common::AsyncOperationSPtr> opvec;
        for (int i = 0; i < PodCnt; i++)
        {
            wstring name(L"TestPod_");
            name += Common::StringUtility::Utf8ToUtf16(to_string(i));
            wstring cgroup(L"");
            bool isdnsserviceenabled = true;
            wstring dnssearchoptions(L"");
            std::wstring hostname(L"myhost");
            std::wstring logdirectory("/var/log/sflogs/");
            std::vector<std::tuple<std::wstring,int,int,int>> portmappings;
            sandboxNameVec.push_back(name);
            auto op = ContainerRuntimeHandler::BeginRunPodSandbox(
                    apphostid, nodeid,
                    name, cgroup,
                    hostname, logdirectory, isdnsserviceenabled
                    dnssearchoptions, portmappings,
                    traceid, TimeSpan::MaxValue,
                    [&](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        cnt++;
                    },
                    AsyncOperationSPtr());
            opvec.push_back(op);
        }
        while (cnt != PodCnt) Sleep(100);
        for (auto op : opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndRunPodSandbox(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Pull Image: ubuntu
        vector<wstring> ImageNames = { L"nginx", L"fedora", L"ubuntu"};
        vector<wstring> ImgCmd = {L"bash"};
        vector<wstring> ImgArg = {};
        vector<pair<wstring, wstring>> ImgEnv;
        vector<tuple<wstring, wstring, bool>> ImgMnts;
        vector<tuple<wstring, wstring>> ImgLabels;
        wstring AuthName;
        wstring AuthPass;

        //AuthName = L"chenxu";
        //AuthPass = L"winfabPass!1";

        for (auto imgName : ImageNames)
        {
            auto op = ContainerRuntimeHandler::BeginPullImage(
                    imgName, AuthName, AuthPass, traceid, TimeSpan::MaxValue,
                    [&](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        cnt++;
                    },
                    AsyncOperationSPtr());
            opvec.push_back(op);
        }
        while (cnt != ImageNames.size()) Sleep(100);
        for (auto op : opvec)
        {
            BOOST_VERIFY(ContainerRuntimeHandler::EndPullImage(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Create Containers
        for (int i = 0; i < PodCnt; i++){
            for (int j = 0; j < ContainerCntPerPod; j++)
            {
                wstring podname = sandboxNameVec[i];
                wstring name(L"TestContainer_");
                name += StringUtility::Utf8ToUtf16(to_string(i)) + L"_" + StringUtility::Utf8ToUtf16(to_string(j));
                containerNameMap[podname].push_back(name);
                auto op = ContainerRuntimeHandler::BeginCreateContainer(
                        apphostid, nodeid,
                        podname, name, ImageNames[j%ImageNames.size()],
                        ImgCmd, ImgArg, ImgEnv, ImgMnts, ImgLabels, L"", L"TestContainer_/application.log",
                        true, traceid, TimeSpan::MaxValue,
                        [&](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            cnt++;
                        },
                        AsyncOperationSPtr());
                opvec.push_back(op);
            }
        }
        while (cnt != PodCnt * ContainerCntPerPod) Sleep(100);
        for (auto op: opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndCreateContainer(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Start Containers
        for (auto pr: containerNameMap)
        {
            wstring sandboxName = pr.first;
            for (auto containerName : pr.second)
            {
                auto op = ContainerRuntimeHandler::BeginStartContainer(
                        containerName, traceid, TimeSpan::MaxValue,
                        [&](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            cnt++;
                        },
                        AsyncOperationSPtr());
                opvec.push_back(op);
            }
        }
        while (cnt != PodCnt * ContainerCntPerPod) Sleep(100);
        for (auto op: opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndStartContainer(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Stop Containers
        for (auto pr: containerNameMap)
        {
            wstring sandboxId = pr.first;
            for (auto containerName : pr.second)
            {
                auto op = ContainerRuntimeHandler::BeginStopContainer(
                        containerName, 0, traceid, TimeSpan::MaxValue,
                        [&](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            cnt++;
                        },
                        AsyncOperationSPtr());
                opvec.push_back(op);
            }
        }
        while (cnt != PodCnt * ContainerCntPerPod) Sleep(100);
        for (auto op: opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndStartContainer(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Remove Containers
        for (auto pr: containerNameMap)
        {
            wstring sandboxId = pr.first;
            for (auto containerName : pr.second)
            {
                auto op = ContainerRuntimeHandler::BeginRemoveContainer(
                        containerName, traceid, TimeSpan::MaxValue,
                        [&](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            cnt++;
                        },
                        AsyncOperationSPtr());
                opvec.push_back(op);
            }
        }
        while (cnt != PodCnt * ContainerCntPerPod) Sleep(100);
        for (auto op: opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndRemoveContainer(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Stop Pods
        for (int i = 0; i < PodCnt; i++)
        {
            auto op = ContainerRuntimeHandler::BeginStopPodSandbox(
                    sandboxNameVec[i], traceid, TimeSpan::MaxValue,
                    [&](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        cnt++;
                    },
                    AsyncOperationSPtr());
            opvec.push_back(op);
        }
        while (cnt != PodCnt) Sleep(100);
        for (auto op : opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndStopPodSandbox(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);

        // Remove Pods
        for (int i = 0; i < PodCnt; i++)
        {
            auto op = ContainerRuntimeHandler::BeginRemovePodSandbox(
                    sandboxNameVec[i], traceid, TimeSpan::MaxValue,
                    [&](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        cnt++;
                    },
                    AsyncOperationSPtr());
            opvec.push_back(op);
        }
        while (cnt != PodCnt) Sleep(100);
        for (auto op : opvec)
        {
            VERIFY_IS_TRUE(ContainerRuntimeHandler::EndRemovePodSandbox(op).IsSuccess());
        }
        opvec.clear();
        cnt.store(0);
#endif
    }

    BOOST_AUTO_TEST_SUITE_END()
}


