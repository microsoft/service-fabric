# Microsoft Azure Service Fabric managed clusters general availability release

Azure Service Fabric managed clusters are now generally available. In this release we are bringing additional capabilities such as availability zone spanning, stateless node types, application secrets, application managed identities, and the ability to configure cluster upgrades. In this release, Service Fabric managed clusters are now available in all Azure public cloud regions and will roll out to Azure Government regions in the coming weeks.

To utilize these new features, you must update the API version in the ARM template to *2021-05-01*. To see all of the properties available in the latest release, see the [ARM template reference](https://docs.microsoft.com/en-us/azure/templates/microsoft.servicefabric/2021-05-01/managedclusters?tabs=json).
 
The following APIs are updated as part of this release:

| API |
|---------|
| Microsoft.ServiceFabric/managedclusters |
| Microsoft.ServiceFabric/managedclusters/nodetypes |
| Microsoft.ServiceFabric/managedclusters/applications |
| Microsoft.ServiceFabric/managedclusters/applicationTypes |
| Microsoft.ServiceFabric/managedclusters/applications/services |

## New features in the GA release

* [Availability zone spanning](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-availability-zones) - You can now deploy Service Fabric managed clusters across availability zones to eliminate single points of failure.
* [Stateless node types](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-stateless-node-type) - Stateless node types enable faster node scaling and VM patching when running services that do not require maintaining any state.
* [Application secrets](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-configuration#enable-automatic-os-image-upgrades) - You can now access encrypted secrets from your Service Fabric application.
* [Application managed identities](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-application-managed-identity) - Application managed identities enable you to access other Azure resources from your application.
* [Cluster upgrade configurations](https://docs.microsoft.com//azure/service-fabric/service-fabric-cluster-upgrade) - You can now select between automatic(default) or manual upgrades and configure an upgrade window to meet the needs of your environment.

## Resources

* [Documentation](https://docs.microsoft.com/azure/service-fabric/overview-managed-cluster)
* [Deployment QuickStart](https://portal.azure.com/#create/Microsoft.Template/uri/https%3A%2F%2Fraw.githubusercontent.com%2FAzure-Samples%2Fservice-fabric-cluster-templates%2Fmaster%2FSF-Managed-Standard-SKU-1-NT%2Fazuredeploy.json) 
* [Sample Cluster Templates](https://github.com/Azure-Samples/service-fabric-cluster-templates) 