// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include <unordered_set>
#include <stack>
#include "State.h"

namespace ModelChecker
{
    class StateSpaceExplorer
    {
    public:
        explicit StateSpaceExplorer(State& initialState);

        void BoundedDfs(int bound);

        static Common::StringLiteral const TraceSource;

    private:
        std::vector<Event> GetEnabledEvents(State& state);

        Event DrawEvent(std::vector<Event>& enabledEvents);

        bool IsNewState(const State& state);

        void UndoLastTransition(State& currentState);

        void Dfs(State& state);

        void DfsIterative();

        State initialState_;

        // Hashtable to prevent revisiting a state
        typedef std::unordered_set<State, StateHash, StateEqual> StateSpace;
        StateSpace stateSpace_; 

        // Contains the events that occurred along current DFS path starting at the initial state;
        typedef std::stack<Event> EventStack;
        EventStack eventStack_;

        int dfsBound_;
    };
}
