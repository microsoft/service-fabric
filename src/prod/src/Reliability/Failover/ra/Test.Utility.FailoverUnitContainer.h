// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class FailoverUnitContainer
            {
                DENY_COPY(FailoverUnitContainer);

            public:
                ~FailoverUnitContainer();

                __declspec(property(get = get_FT)) FailoverUnit & FT;
                FailoverUnit & get_FT() { return *ft_; }

                __declspec(property(get = get_Entry)) Infrastructure::LocalFailoverUnitMapEntrySPtr const & Entry;
                Infrastructure::LocalFailoverUnitMapEntrySPtr const & get_Entry() const { return entry_; }

                void InsertIntoLFUM(UnitTestContext & utContext);

                // Create a FT with a new FT Context
                // Useful for creating a separate FT
                static bool Create(
                    std::wstring const & ftString,
                    StateManagement::ReadContext const & readContext,
                    FailoverUnitContainerUPtr & rv);

                static FailoverUnitContainerUPtr CreateClosedFT(StateManagement::SingleFTReadContext const & context);
                static FailoverUnitContainerUPtr CreateClosedFT(std::wstring const & shortName, UnitTestContext & readContext);

                static FailoverUnitSPtr CreateClosedFTPtr(std::wstring const & shortName);
                static FailoverUnitSPtr CreateClosedFTPtr(StateManagement::SingleFTReadContext const & readContext);

                static void Compare(FailoverUnit & ftExpected, FailoverUnit & ftActual);

            private:

                void ReconcileAllSets(ReconfigurationAgent & ra);
                void ReconcileSet(Infrastructure::SetMembershipFlag const & flag, Infrastructure::StateMachineActionQueue & queue);

                static void CompareReplicas(Replica const & expected, Replica const & actual, size_t replicaIndex);

                static void CompareFMMessageState(FMMessageState const & expected, FMMessageState const & actual);

                FailoverUnitContainer(FailoverUnitSPtr && ft);

                FailoverUnitSPtr ftSPtr_;
                Infrastructure::LocalFailoverUnitMapEntrySPtr entry_;
                FailoverUnit* ft_;
            };
        }
    }
}

