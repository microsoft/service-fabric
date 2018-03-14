// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    
    struct PlbMovementIgnoredReasons
    {
    public :
        enum Enum
        {
            Invalid = 0,
            ClusterPaused = 1,
            DropAllPLBMovementsConfigTrue = 2,
            FailoverUnitNotFound = 3,
            FailoverUnitIsToBeDeleted = 4,
            VersionMismatch = 5,
            FailoverUnitIsChanging = 6,
            NodePendingClose = 7,
            NodeIsUploadingReplicas = 8,
            ReplicaNotDeleted = 9,
            NodePendingDeactivate = 10,
            InvalidReplicaState = 11,
            CurrentConfigurationEmpty = 12,
            ReplicaConfigurationChanging = 13,
            ToBePromotedReplicaAlreadyExists = 14,
            ReplicaToBeDropped = 15,
            ReplicaNotFound = 16,
            PrimaryNotFound = 17,
            LastValidEnum = PrimaryNotFound
        };

        PlbMovementIgnoredReasons();
        PlbMovementIgnoredReasons(Enum reason);
        ~PlbMovementIgnoredReasons() = default;
        PlbMovementIgnoredReasons(const PlbMovementIgnoredReasons & other);
        PlbMovementIgnoredReasons & operator=(PlbMovementIgnoredReasons const& other);

        __declspec (property(get = get_Reason, put = set_Reason)) Enum Reason;
        Enum get_Reason() const { return reason_; }
        void set_Reason(Enum action) { reason_ = action; }

        std::wstring ToString(Enum const & val) const;
        void FillEventData(Common::TraceEventContext& context) const;
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const & format) const;
        

        static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);

    private :
        class Initializer
        {
        public:
            Initializer();
        };
        static Common::Global<Initializer> GlobalInitializer;

        Enum reason_;

    };


}
