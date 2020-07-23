set( LINUX_SOURCES
  ../Accumulator.cpp
  ../AccumulatorWithMinMax.cpp
  ../AffinityConstraint.cpp
  ../Application.cpp
  ../ApplicationCapacitiesDescription.cpp
  ../ApplicationCapacityConstraint.cpp
  ../ApplicationDescription.cpp
  ../ApplicationEntry.cpp
  ../ApplicationLoad.cpp
  ../ApplicationNodeCount.cpp
  ../ApplicationNodeLoads.cpp
  ../ApplicationReservedLoads.cpp
  ../ApplicationPlacement.cpp
  ../ApplicationTotalLoad.cpp
  ../AutoScaleStatistics.cpp
  ../AutoScaler.cpp
  ../BalanceChecker.cpp
  ../BalanceCheckerCreator.cpp
  ../BoundedSet.cpp
  ../CandidateSolution.cpp
  ../Checker.cpp
  ../DiagnosticsDataStructures.cpp
  ../DynamicNodeLoadSet.cpp
  ../DynamicBitSet.cpp
  ../FailoverUnit.cpp
  ../FailoverUnitDescription.cpp
  ../FailoverUnitMovementType.cpp
  ../FailoverUnitMovement.cpp
  ../FaultDomainConstraint.cpp
  ../IConstraint.cpp
  ../IntervalCounter.cpp
  ../IPlacementAndLoadBalancing.cpp
  ../IViolation.cpp
  ../LoadBalancingDomainEntry.cpp
  ../LoadBalancing.PerformanceCounters.cpp
  ../LoadEntry.cpp
  ../Metric.cpp
  ../MetricGraph.cpp
  ../Movement.cpp
  ../MovePlan.cpp
  ../Node.cpp
  ../NodeCapacityConstraint.cpp
  ../NodeDescription.cpp
  ../NodeBlockListConstraint.cpp
  ../NodeEntry.cpp
  ../NodeMetrics.cpp
  ../NodeSet.cpp
  ../PartitionClosure.cpp
  ../PartitionDomainStructure.cpp
  ../PartitionEntry.cpp
  ../PartitionPlacement.cpp
  ../Placement.cpp
  ../PlacementCreator.cpp
  ../PlacementAndLoadBalancing.cpp
  ../PlacementAndLoadBalancingTestHelper.cpp
  ../PlacementReplica.cpp
  ../PLBFailoverUnitDescriptionFlags.cpp
  ../PLBScheduler.cpp
  ../PLBSchedulerActionType.cpp
  ../PLBSchedulerAction.cpp
  ../PLBStatistics.cpp
  ../PreferredLocationConstraint.cpp
  ../RandomDistribution.cpp
  ../ReplicaExclusionConstraint.cpp
  ../ReplicaPlacement.cpp
  ../ReplicaSet.cpp
  ../ReservationLoad.cpp
  ../ScaleoutCountConstraint.cpp
  ../Score.cpp
  ../Searcher.cpp
  ../Service.cpp
  ../ServiceDescription.cpp
  ../ServiceDomain.cpp
  ../ServiceDomainLocalMetric.cpp
  ../ServiceDomainMetric.cpp
  ../ServicePackageNode.cpp
  ../ServiceEntry.cpp
  ../ServiceMetric.cpp
  ../ServiceType.cpp
  ../ServiceTypeDescription.cpp
  ../Snapshot.cpp
  ../SearchInsight.cpp
  ../SystemState.cpp
  ../TempSolution.cpp
  ../TimedCounter.cpp
  ../tracehelpers.cpp
  ../ViolationList.cpp
  ../PLBDiagnostics.cpp
  ../DiagnosticSubspace.cpp
  ../ServicePackageEntry.cpp
  ../ServicePackagePlacement.cpp
  ../ServicePackageDescription.cpp
  ../ServicePackage.cpp
  ../RGStatistics.cpp
  ../DefragStatistics.cpp
  ../InBuildCountPerNode.cpp
  ../ThrottlingConstraint.cpp
)

include_directories("..")

add_library(objects_LoadBalancing OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_LoadBalancing ../stdafx.h)

add_library(${lib_LoadBalancing} STATIC $<TARGET_OBJECTS:objects_LoadBalancing>)

target_link_libraries( ${lib_LoadBalancing}
    ${lib_LoadBalancingCommon}
    )

set_target_properties(${lib_LoadBalancing} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

