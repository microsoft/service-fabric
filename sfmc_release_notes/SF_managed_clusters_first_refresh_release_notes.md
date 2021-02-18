# Microsoft Azure Service Fabric managed clusters "2021-01-01-preview" release (Preview)

The first refresh release for Microsoft Azure Service Fabric managed clusters is now available. We first introduced Service Fabric managed clusters in October, 2020. Service Fabric managed clusters enable customers to deploy Service Fabric clusters as a single ARM resource, and reduce the complexity associated with common cluster operations such as scaling, and certificate rollovers. In this refresh we are bringing additional capabilities such as ARM application deployments, automatic OS upgrades, and Virtual Machine Scale Set managed identities that enable you to focus on application development, and deployment.

To utilize these new features, you must update the API version in the ARM template to *2021-01-01-preview*.

**This release is currently available in the following regions; East Asia, North Europe, West Central US, East US2.** Additional regions will be added in future updates.  

The following APIs are updated as part of this release:

| API |
|---------|
| Microsoft.ServiceFabric/managedclusters |
| Microsoft.ServiceFabric/managedclusters/nodetypes |
| Microsoft.ServiceFabric/managedclusters/applications |
| Microsoft.ServiceFabric/managedclusters/applicationTypes |
| Microsoft.ServiceFabric/managedclusters/applications/services |

## Features

* [ARM application deployments](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-app-deployment-template) - You can now use ARM to deploy applications to Service Fabric managed clusters. Using ARM provides greater control of resource properties, and ensures consistent resource model. With ARM application deployments, you do not have to wait for the cluster to be ready before deploying; application registration, provisioning, and deployment can all happen in one step.
* [Disk encryption](https://docs.microsoft.com/azure/service-fabric/how-to-enable-managed-cluster-disk-encryption?tabs=azure-powershell) - Disk encryption can now be enabled on nodes in a Service Fabric managed cluster to meet compliance requirements.
* [Automatic OS image upgrades](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-configuration#enable-automatic-os-image-upgrades) - Virtual Machine Scale Set automatic OS image upgrades for your Service Fabric managed cluster are now available. If a new OS version is available, the cluster node types will be upgraded one at a time. With this feature you have the option for your cluster node types to automatically be upgraded when a new OS version is available.
* [Virtual Machine Scale Set Managed Identities](https://docs.microsoft.com/azure/service-fabric/how-to-managed-identity-managed-cluster-virtual-machine-scale-sets) - Each node type in a Service Fabric managed cluster is backed by a Virtual Machine Scale Set. To enable managed identities to be used with a managed cluster node type, a property vmManagedIdentity has been added to node type definitions. This is a list that contains user defined identities such as Azure userAssignedIdenties.  
* [NSG rule configuration](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration#networking-configurations) - You can now customize Network Security Group rules enabling you to meet any compliance requirements for your scenarios.

## Known Issues

### ARM application deployments

* Stateless service instance count cannot be updated after initial deployment.
* When redeploying an application, the services must first be deleted through ARM.
* When using the PowerShell cmdlets, the GET application command may fail. If you experience this issue, you can alternatively use the Azure CLI. 

## Upcoming Features

* Application managed identities for apps deployed to a Service Fabric managed cluster.
* Service Fabric managed clusters which span across availability zones.
* Auto scale the number of nodes in a cluster based on performance and load metrics.
* Azure portal support for create and manage experiences.
* Reverse proxy support.
* Manual upgrade mode support.

For more info, see the [Service Fabric managed cluster project](https://github.com/microsoft/service-fabric/projects/17).

## Resources

* [Documentation](https://docs.microsoft.com/azure/service-fabric/overview-managed-cluster)
* [Deployment QuickStart](https://portal.azure.com/#create/Microsoft.Template/uri/https%3A%2F%2Fraw.githubusercontent.com%2FAzure-Samples%2Fservice-fabric-cluster-templates%2Fmaster%2FSF-Managed-Standard-SKU-1-NT%2Fazuredeploy.json) 
* [Sample Cluster Templates](https://github.com/Azure-Samples/service-fabric-cluster-templates) 