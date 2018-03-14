// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            // Traits definition for the entity
            template <typename T>
            struct EntityTraits
            {
                // DataType: typedef for the class that stores the state (ex: FailoverUnit)
                // IdType: typedef for the class that identifies this object (ex: FailoverUnitId)
                // HandlerParametersType: typedef for the class that defines job item handler parameters 
                
                // RowType: Storage::Api::RowType::Enum which defines what is the type of this object in the store

                // AddEntityIdToMessage: A function that can add entity id to a transport message with the correct header

                // GetEntityMap: A function that can return the map that stores this entity in the RA

                // CreateHandlerParameters: A function that can create the job item handler parameters

                // PerformChecks: A function that can execute job item specific checks specific for this entity

                // AssertInvariants: A function that can assert that all the invariants on the data type are satisfied

                // MessageStalenessChecker: Return an instance of the staleness checker for messages

                // Create: factory functions for creating the entity

                // Trace: Knows how to trace this entity

                // GetEntityIdFromMessage: Returns the id from the message
            };
        }
    }
}
