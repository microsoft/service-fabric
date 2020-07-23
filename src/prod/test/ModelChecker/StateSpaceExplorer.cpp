// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "StateSpaceExplorer.h"

using namespace std;
using namespace ModelChecker;

Common::StringLiteral const StateSpaceExplorer::TraceSource = "ModelChecker";

StateSpaceExplorer::StateSpaceExplorer(State& initialState)
    : initialState_(initialState)
{
}

void StateSpaceExplorer::UndoLastTransition(State& currentState)
{
    EventStack::container_type eventContainer = eventStack_._Get_container();
    currentState = initialState_;
    for (auto Iter = eventContainer.begin(); Iter != eventContainer.end(); Iter++)
    {
        Event event( Iter->EventType, Iter->MessageBodyStr );
        currentState.MakeTransition(event);
    }
}

std::vector<Event> StateSpaceExplorer::GetEnabledEvents(State& state)
{
    // Could use more sophisticated logic for deciding which events should be enabled
    // like: partial order reduction (see https://www.usenix.org/legacy/events/nsdi09/tech/full_papers/yang/yang_html/index.html)
    return state.EventList;
}

Event StateSpaceExplorer::DrawEvent(std::vector<Event>& enabledEvents)
{
    // Could also draw from a suitable probability distribution
    Event event = enabledEvents[0];
    enabledEvents.erase( enabledEvents.begin() );
    return event;
}

bool StateSpaceExplorer::IsNewState(const State& state) 
{
    State copyOfCurrentState(state);
    std::pair<StateSpace::iterator, bool> status = stateSpace_.insert(copyOfCurrentState);
    // was insertion successful?
    //wstring statusString = status.second ? L"New State" : L"Old State";
    //TestCommon::TestSession::WriteInfo(TraceSource, "{0}", statusString);
    return status.second;
}

void StateSpaceExplorer::Dfs(State& state)
{
    static int depth = 0;

    //TestSession::WriteInfo(TraceSource, "ENTERING DFS DEPTH: {0}", depth);

    if (depth >= dfsBound_)
    {
        //TestSession::WriteInfo(TraceSource, "TOO DEEP! @{0}", depth);
        return;
    }

    if (!IsNewState(state)) 
    {
        return;
    }

    vector<Event> enabledEvents = GetEnabledEvents(state);

    while (enabledEvents.size() > 0) 
    {
        Event event = DrawEvent(enabledEvents);

        eventStack_.push(event);
        //TestSession::WriteInfo(TraceSource, "Taking {0}", event.ToString());
        state.MakeTransition(event);
        ++depth;
        //TestSession::WriteInfo(TraceSource, "Now at depth = {0}", depth);
        
        Dfs(state);
        
        eventStack_.pop();
        //TestSession::WriteInfo(TraceSource, "Undoing {0}", event.ToString());
        UndoLastTransition(state); 
        --depth;
        //TestSession::WriteInfo(TraceSource, "Going back to depth = {0}", depth);
    }
}

void StateSpaceExplorer::DfsIterative()
{
    // Currently it does not track the event-history of a state
    typedef std::stack<std::pair<State, int>> DfsStack;
    DfsStack dfsStack;

    State copyOfInitialState(initialState_);
    int depth = 0;
    
    dfsStack.push( make_pair(copyOfInitialState, depth) );

    while(dfsStack.size() > 0)
    {
        pair<State, int> stateAndDepth = move(dfsStack.top());
        dfsStack.pop();

        State currentState(stateAndDepth.first);
        depth = stateAndDepth.second;

        if(IsNewState(currentState))
        {
            vector<Event> enabledEvents = GetEnabledEvents(currentState);
            State copyOfCurrentState(currentState);

            while(enabledEvents.size() > 0)
            {
                Event event = DrawEvent(enabledEvents);

                //TestCommon::TestSession::WriteInfo(TraceSource, "Taking {0}", event.ToString());

                currentState.MakeTransition(event);

                if((depth+1) < dfsBound_)
                {
                    dfsStack.push(make_pair(currentState, (depth+1)));
                }
                else
                {
                    //TestCommon::TestSession::WriteInfo(TraceSource, "TOO DEEP! @{0}", depth+1);
                }

                //TestCommon::TestSession::WriteInfo(TraceSource, "Undoing {0}", event.ToString());

                currentState = copyOfCurrentState;
            }
        }
    }
}

void StateSpaceExplorer::BoundedDfs(int bound)
{
    dfsBound_ = bound;
    //State copyOfInitialState(initialState_);
    stateSpace_.clear();
    //Dfs(copyOfInitialState);
    DfsIterative();
    TestCommon::TestSession::WriteInfo(TraceSource, "{0} new states explored.", stateSpace_.size());
}
