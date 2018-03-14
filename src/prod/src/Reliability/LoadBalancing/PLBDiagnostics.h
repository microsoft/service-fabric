// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PlacementReplica.h"
#include "stdafx.h"
#include "client/ClientPointers.h"
#include "IConstraint.h"
#include "Node.h"
#include "NodeSet.h"
#include "DiagnosticSubspace.h"
#include "PLBEventSource.h"
#include "CandidateSolution.h"
#include "ServiceDomain.h"
#include "DiagnosticsDataStructures.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PLBDiagnostics;

        typedef std::wstring ReplicaKeyType;
        //  <ReplicaEntry, <NumberOfFailedPlacementAttempts, ReasonForFailedPlacement>>
        typedef std::map<ReplicaKeyType, std::pair<size_t, std::wstring>> ReplicaDiagnosticMap;
        // <ServiceName, <ReplicaEntry, <NumberOfFailedPlacementAttempts, ReasonForFailedPlacement>>>
        typedef std::map<std::wstring, ReplicaDiagnosticMap> PlacementDiagnosticsTable;
        typedef std::shared_ptr<PlacementDiagnosticsTable> PlacementDiagnosticsTableSPtr;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;


        typedef Common::Guid PartitionKeyType;
        //  <ParitionEntry, <NumberOfFailedPlacementAttempts, ReasonForFailedPlacement>>
        typedef std::map<PartitionKeyType, std::pair<size_t, std::wstring>> PartitionDiagnosticMap;
        // <ServiceName, <ParitionEntry, <NumberOfFailedPlacementAttempts, ReasonForFailedPlacement>>>
        typedef std::map<std::wstring, PartitionDiagnosticMap> PartitionDiagnosticsTable;
        typedef std::shared_ptr<PartitionDiagnosticsTable> PartitionDiagnosticsTableSPtr;

        typedef std::map<Common::Guid, size_t> FMConsecutiveDroppedPLBMovementsTable;
        typedef std::shared_ptr<FMConsecutiveDroppedPLBMovementsTable> FMConsecutiveDroppedPLBMovementsTableSPtr;

        typedef std::shared_ptr<SchedulingDiagnostics> SchedulingDiagnosticsSPtr;

        class PLBDiagnostics
        {
            DENY_COPY_ASSIGNMENT(PLBDiagnostics);

        public:
            typedef std::wstring DomainId;
            typedef std::map<ServiceDomain::DomainId, ServiceDomain> ServiceDomainTable;

            PLBDiagnostics(Client::HealthReportingComponentSPtr healthClient,
                bool const & verbosePlacementHealthReportingEnabled,
                std::vector<Node> const & plbNodes,
                ServiceDomainTable const & serviceDomainTable,
                PLBEventSource const& trace);

            __declspec (property(get = get_VerbosePlacementHealthReportingEnabled)) bool VerbosePlacementHealthReportingEnabled;
            bool get_VerbosePlacementHealthReportingEnabled() const { return verbosePlacementHealthReportingEnabled_; }

            __declspec (property(get = get_ConstraintDiagnosticsReplicas)) std::vector<ConstraintViolation> const & ConstraintDiagnosticsReplicas;
            std::vector<ConstraintViolation> const & get_ConstraintDiagnosticsReplicas() const { return constraintDiagnosticsReplicas_; }

            __declspec (property(get = get_SchedulingDiagnostics)) SchedulingDiagnosticsSPtr const & SchedulersDiagnostics;
            SchedulingDiagnosticsSPtr const & get_SchedulingDiagnostics() const { return schedulingDiagnosticsSPtr_; }

            __declspec (property(get = get_PLBNodes)) std::vector<Node> const & PLBNodes;
            std::vector<Node> const & get_PLBNodes() const { return plbNodes_; }

            static std::wstring PartitionDetailString(PartitionEntry const* p);
            static std::wstring ReplicaDetailString(PlacementReplica const* r);
            std::wstring NodeSetDetailString(NodeSet nodes, NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr);
            std::wstring ServiceDomainMetricsString(DomainId const & domainId, bool forProperty);

            static std::wstring ShortFriendlyConstraintName(IConstraint::Enum type);
            static std::wstring FriendlyConstraintName(IConstraint::Enum type, PlacementReplica const* r, PartitionEntry const* p);

            static void GenerateUnplacedTraceandNodeDetailMessages(
                std::vector<IConstraintUPtr> const & constraints,
                std::vector<DiagnosticSubspace> & subspaces,
                PlacementReplica const* r,
                size_t totalNodeCount,
                std::wstring & traceMessage,
                std::wstring & nodeDetailMessage,
                bool trackEliminatedNodes,
                bool plbMovementsBeingDropped);

            static void GenerateUnswappedUpgradeTraceMessage(
                std::vector<IConstraintUPtr> const & constraints,
                std::vector<DiagnosticSubspace> & subspaces,
                PartitionEntry const* p,
                size_t totalNodeCount,
                std::wstring & traceMessage,
                std::wstring & nodeDetailMessage,
                bool trackEliminatedNodes,
                bool plbMovementsBeingDropped);

            void TrackConstraintViolation(PlacementReplica const* itReplica, IConstraintUPtr const & it, std::shared_ptr<IConstraintDiagnosticsData> diagnosticsDataSPtr);
            void TrackBalancingFailure(CandidateSolution & oldSolution, CandidateSolution & newSolution, DomainId const & domainId);
            void TrackNodeCapacityViolation(std::function< Common::ErrorCode (Federation::NodeId, ServiceModel::NodeLoadInformationQueryResult &)> nodeLoadPopulator);

            bool TrackUnplacedReplica(PlacementReplica const* r, std::wstring & traceMessage, std::wstring & nodeDetailMessage);
            bool TrackUnfixableConstraint(ConstraintViolation const & constraintViolation, std::wstring & traceMessage, std::wstring & nodeDetailMessage);
            bool UpdateConstraintFixFailureLedger(PlacementReplica const* itReplica, IConstraintUPtr const & it);
            bool TrackEliminatedNodes(PlacementReplica const* r);
            void TrackPlacedReplica(PlacementReplica const* r);

            bool TrackUpgradeSwapEliminatedNodes(PartitionEntry const* p);
            bool TrackConstraintFixEliminatedNodes(PlacementReplica const* r, IConstraint::Enum it);
            void TrackUpgradeSwap(PartitionEntry const* p);
            bool TrackUpgradeUnswappedPartition(PartitionEntry const* p, std::wstring & traceMessage, std::wstring & nodeDetailMessage);

            void TrackDroppedPLBMovement(Common::Guid const & partitionId);
            void TrackExecutedPLBMovement(Common::Guid const & partitionId);
            bool IsPLBMovementBeingDropped(Common::Guid const & partitionId);

            void ClearDiagnosticsTables();
            void CleanupSearchForPlacementDiagnostics();

            void CleanupConstraintDiagnosticsCaches();

            void ReportSearchForPlacementHealth();
            void ReportConstraintHealth();
            void ReportBalancingHealth();

            void ReportDroppedPLBMovementsHealth();

            void ReportServiceCorrelationError(ServiceDescription const& service);
            void ReportServiceDescriptionOK(ServiceDescription const& service);

            void RemoveService(std::wstring && serviceName);
            void RemoveFailoverUnit(std::wstring && serviceName, Common::Guid const & fUId);

            void ExecuteUnderPDTLock(std::function<void()> lockedOperation);
            void ExecuteUnderUSPDTLock(std::function<void()> lockedOperation);

            std::vector<std::wstring> GetUnplacedReplicaInformation(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries);

            __declspec (property(get=get_Trace)) PLBEventSource const& Trace;
            PLBEventSource const& get_Trace() const {return trace_;}

        private:

            Common::RwLock serviceDiagnosticLock_;
            Common::RwLock partitionDiagnosticLock_;
            Common::RwLock placementDiagnosticsTableLock_;
            Common::RwLock upgradeSwapDiagnosticsTableLock_;
            Common::RwLock droppedMovementTableLock_;
            Common::RwLock queryLock_;

            std::wstring ConstraintDetails(IConstraint::Enum type, PlacementReplica const* r, std::shared_ptr<IConstraintDiagnosticsData> diagnosticsDataSPtr);

            static bool ReportAsHealthWarning(PlacementReplica const* itReplica, IConstraintUPtr const & it);

            void ReportServiceHealth(
                std::wstring && reportingName,
                ServiceDescription const & serviceDescription,
                Common::SystemHealthReportCode::Enum reportCode,
                std::wstring && healthDetails,
                Common::TimeSpan ttl);

            //This reference is bound to and mimics any changes in the plb's version of this variable, which can be toggled through powershell
            bool const & verbosePlacementHealthReportingEnabled_;
            //This reference is bound to and mimics any changes in the plb's version of the nodes_
            std::vector<Node> const & plbNodes_;
            ServiceDomainTable const & plbServiceDomainTable_;
            Client::HealthReportingComponentSPtr healthClient_;
            std::vector<ServiceModel::HealthReport> constraintViolationHealthReports_;
            std::vector<ServiceModel::HealthReport> unfixableConstraintViolationHealthReports_;
            std::vector<ServiceModel::HealthReport> unplacedReplicaHealthReports_;
            std::vector<ServiceModel::HealthReport> unswappableUpgradePartitionHealthReports_;
            std::vector<ServiceModel::HealthReport> balancingViolationHealthReports_;
            std::vector<ServiceModel::HealthReport> consecutiveDroppedMovementsHealthReports_;
            std::queue<std::pair<std::wstring, std::wstring>> placementQueue_;
            std::vector<ConstraintViolation> constraintDiagnosticsReplicas_;
            std::queue<std::pair<std::wstring, Common::Guid>> upgradeSwapQueue_;
            std::queue<std::pair<std::wstring, Common::Guid>> removedFUQueue_;
            std::queue<std::wstring> removedServiceQueue_;
            PlacementDiagnosticsTableSPtr placementDiagnosticsTableSPtr_;
            PlacementDiagnosticsTableSPtr constraintDiagnosticsTableSPtr_;
            PlacementDiagnosticsTableSPtr cachedConstraintDiagnosticsTableSPtr_;
            PlacementDiagnosticsTableSPtr queryableDiagnosticsTableSPtr_;
            PartitionDiagnosticsTableSPtr upgradeSwapDiagnosticsTableSPtr_;
            PartitionDiagnosticsTableSPtr queryableUpgradeSwapDiagnosticsTableSPtr_;
            FMConsecutiveDroppedPLBMovementsTableSPtr fmConsecutiveDroppedPLBMovementsTableSPtr_;
            SchedulingDiagnosticsSPtr schedulingDiagnosticsSPtr_;

            PLBEventSource const& trace_;

        };
    }
}
