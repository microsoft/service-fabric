// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"

namespace Federation
{
    class StateMachineTestInfo
    {
    public:
        StateMachineTestInfo(
            std::wstring name,
            std::wstring initialState,
            std::wstring triggerEvent,
            std::wstring finalState,
            std::wstring actionsTaken)
            : name_(name), 
            initialState_(initialState), 
            triggerEvent_(triggerEvent), 
            finalState_(finalState),
            actionsTaken_(actionsTaken)
        {
        }

        __declspec(property(get=getName)) std::wstring Name;
        __declspec(property(get=getInitialState)) std::wstring InitialState;
        __declspec(property(get=getTriggerEvent)) std::wstring TriggerEvent;
        __declspec(property(get=getFinalState)) std::wstring FinalState;
        __declspec(property(get=getActionsTaken)) std::wstring ActionsTaken;

        std::wstring getName() const { return name_; }
        std::wstring getInitialState() const { return initialState_; }
        std::wstring getTriggerEvent() const { return triggerEvent_; }
        std::wstring getFinalState() const { return finalState_; }
        std::wstring getActionsTaken() const { return actionsTaken_; }

    private:
        std::wstring name_;
        std::wstring initialState_;
        std::wstring triggerEvent_;
        std::wstring finalState_;
        std::wstring actionsTaken_;
    };

    class TestNodeInfo
    {
    public:
        TestNodeInfo()
        {
        }

        TestNodeInfo(NodeId id, NodePhase::Enum phase)
            : nodeId_(id), phase_(phase)
        {
        }

        static bool Create(std::wstring string, __out TestNodeInfo & testNode);

        _declspec(property(get=getNodeId)) NodeId Id;
        _declspec(property(get=getPhase)) NodePhase::Enum Phase;

        NodeId getNodeId() const { return nodeId_; }
        NodePhase::Enum getPhase() const { return phase_; }

        std::wstring ToString();
    private:
        NodeId nodeId_;
        NodePhase::Enum phase_;
    };

    class TestVoteInfo
    {
         public:
        TestVoteInfo()
        {
        }

        TestVoteInfo(NodeId voteId, Common::StopwatchTime superTicket, Common::StopwatchTime globalTicket)
            : voteId_(voteId), superTicket_(superTicket), globalTicket_(globalTicket)
        {
        }

        static bool Create(std::wstring string, __out TestVoteInfo & testVote);

        _declspec(property(get=getId)) NodeId Id;
        _declspec(property(get=getSuperTicket)) Common::StopwatchTime SuperTicket;
        _declspec(property(get=getGlobalTicket)) Common::StopwatchTime GlobalTicket;

        NodeId getId() const { return voteId_; }
        Common::StopwatchTime getSuperTicket() const { return superTicket_; }
        Common::StopwatchTime getGlobalTicket() const { return globalTicket_; }

        std::wstring ToString();
    private:
        NodeId voteId_;
        Common::StopwatchTime superTicket_;
        Common::StopwatchTime globalTicket_;
    };
};
