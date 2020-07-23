set( LINUX_SOURCES
    ../ApplicationContext.cpp
    ../ApplicationDescription.cpp
    ../ApplicationTypeContext.cpp
    ../ApplicationTypeRequestTracker.cpp
    ../applicationupdatecontext.cpp
    ../ApplicationUpgradeContext.cpp
    ../ClearApplicationUpgradeContextAsyncOperation.cpp
    ../ClearComposeDeploymentUpgradeContextAsyncOperation.cpp
    ../ClearSingleInstanceDeploymentUpgradeContextAsyncOperation.cpp
    ../ClientRequestAsyncOperation.cpp
    ../ClusterBaselineAsyncOperation.cpp
    ../ClusterManagerCounters.cpp
    ../ClusterManagerEventSource.cpp
    ../ClusterManagerFactory.cpp
    ../ClusterManagerMessage.cpp
    ../ClusterManagerQueryMessageHandler.cpp
    ../ClusterManagerReplica.cpp
    ../ComposeDeploymentContext.cpp
    ../ComposeDeploymentUpgradeContext.cpp
    ../Constants.cpp
    ../ContainerGroupAppTypeNameVersionGenerator.cpp
    ../CreateApplicationAsyncOperation.cpp
    ../CreateComposeDeploymentAsyncOperation.cpp
    ../CreateDefaultServiceWithDnsNameIfNeededAsyncOperation.cpp
    ../CreateServiceAsyncOperation.cpp
    ../CreateServiceFromTemplateAsyncOperation.cpp
    ../CreateSingleInstanceApplicationAsyncOperation.cpp
    ../CreateVolumeAsyncOperation.cpp
    ../DeletableRolloutContext.cpp
    ../DeleteApplication2AsyncOperation.cpp
    ../DeleteApplicationAsyncOperation.cpp
    ../DeleteApplicationContextAsyncOperation.cpp
    ../DeleteApplicationTypeContextAsyncOperation.cpp
    ../DeleteComposeDeploymentAsyncOperation.cpp
    ../DeleteComposeDeploymentContextAsyncOperation.cpp
    ../DeleteServiceAsyncOperation.cpp
    ../DeleteServiceContextAsyncOperation.cpp
    ../DeleteSingleInstanceDeploymentAsyncOperation.cpp
    ../DeleteSingleInstanceDeploymentContextAsyncOperation.cpp
    ../DeleteVolumeAsyncOperation.cpp
    ../DeploymentType.cpp
    ../DigestedApplicationDescription.cpp
    ../DockerComposeAppTypeNameVersionGenerator.cpp
    ../FabricClientProxy.cpp
    ../FabricProvisionContext.cpp
    ../FabricUpgradeContext.cpp
    ../FinishInfrastructureTaskAsyncOperation.cpp
    ../GetFabricUpgradeStatusAsyncOperation.cpp
    ../GetUpgradeStatusAsyncOperation.cpp
    ../GoalStateApplicationUpgradeContext.cpp
    ../ImageBuilderAsyncOperationExecutor.cpp
    ../ImageBuilderProxy.cpp
    ../InfrastructureTaskContext.cpp
    ../MigrateDataAsyncOperation.cpp
    ../MoveNextFabricUpgradeDomainAsyncOperation.cpp
    ../MoveNextUpgradeDomainAsyncOperation.cpp
    ../ProcessApplicationContextAsyncOperation.cpp
    ../ProcessApplicationContextUpdateAsyncOperation.cpp
    ../ProcessApplicationTypeContextAsyncOperation.cpp
    ../ProcessApplicationUpgradeContextAsyncOperation.cpp
    ../ProcessComposeDeploymentContextAsyncOperation.cpp
    ../ProcessComposeDeploymentUpgradeContextAsyncOperation.cpp
    ../ProcessFabricProvisionContextAsyncOperation.cpp
    ../ProcessFabricUpgradeContextAsyncOperation.cpp
    ../ProcessInfrastructureTaskContextAsyncOperation.cpp
    ../ProcessProvisionContextAsyncOperationBase.cpp
    ../ProcessQueryAsyncOperation.cpp
    ../ProcessRolloutContextAsyncOperationBase.cpp
    ../ProcessServiceContextAsyncOperation.cpp
    ../ProcessSingleInstanceDeploymentContextAsyncOperation.cpp
    ../ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation.cpp
    ../ProcessUpgradeContextAsyncOperationBase.cpp
    ../ProvisionApplicationTypeAsyncOperation.cpp
    ../ProvisionFabricAsyncOperation.cpp
    ../ReplaceSingleInstanceDeploymentContextAsyncOperation.cpp
    ../ReportFabricUpgradeHealthAsyncOperation.cpp
    ../ReportUpgradeHealthAsyncOperation.cpp
    ../RollbackApplicationUpgradeAsyncOperation.cpp
    ../RollbackComposeDeploymentAsyncOperation.cpp
    ../RollbackFabricUpgradeAsyncOperation.cpp
    ../RolloutContext.cpp
    ../RolloutContextType.cpp
    ../RolloutManager.cpp
    ../RolloutManagerState.cpp
    ../RolloutStatus.cpp
    ../ServiceContext.cpp
    ../ServiceModelServiceManifestDescription.cpp
    ../ServiceModelTypeName.cpp
    ../SingleInstanceDeploymentContext.cpp
    ../SingleInstanceDeploymentUpgradeContext.cpp
    ../StartInfrastructureTaskAsyncOperation.cpp
    ../StoreDataApplicationIdMap.cpp
    ../StoreDataApplicationManifest.cpp
    ../StoreDataClusterUpgradeStateSnapshot.cpp
    ../StoreDataComposeDeploymentInstance.cpp
    ../StoreDataGlobalInstanceCounter.cpp
    ../StoreDataServiceManifest.cpp
    ../StoreDataServicePackage.cpp
    ../StoreDataServiceTemplate.cpp
    ../StoreDataSingleInstanceApplicationInstance.cpp
    ../StoreDataSingleInstanceDeploymentCounter.cpp
    ../StoreDataVerifiedFabricUpgradeDomains.cpp
    ../StoreDataVerifiedUpgradeDomains.cpp
    ../StoreDataVolume.cpp
    ../TestDockerComposeImageBuilderProxy.cpp
    ../UnprovisionApplicationTypeAsyncOperation.cpp
    ../UnprovisionFabricAsyncOperation.cpp
    ../UpdateApplicationAsyncOperation.cpp
    ../UpdateApplicationUpgradeAsyncOperation.cpp
    ../UpdateFabricUpgradeAsyncOperation.cpp
    ../UpgradeApplicationAsyncOperation.cpp
    ../UpgradeComposeDeploymentAsyncOperation.cpp
    ../UpgradeFabricAsyncOperation.cpp
    ../VolumeManager.cpp
)

include_directories("..")

add_library(objects_ClusterManager OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ClusterManager ../stdafx.h)

add_library(${lib_ClusterManager} STATIC $<TARGET_OBJECTS:objects_ClusterManager>)

set_target_properties(${lib_ClusterManager} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

