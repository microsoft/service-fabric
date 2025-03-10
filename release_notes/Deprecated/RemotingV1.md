# Service Fabric Remoting V1 Deprecation Strategy

The V1 version of Service Fabric Remoting protocol uses BinaryFormatter for message and exception serialization. Because BinaryFormatter relies on type
information embedded in the serialized data during deserialization, this allows an attacker to make deserializer execute arbitrary code and is inherently
unsafe. The BinaryFormatter APIs were marked obsolete in .NET 5 and removed from .NET 9. For details see
[BinaryFormatter Obsoletion Strategy](https://github.com/dotnet/designs/blob/main/accepted/2020/better-obsoletion/binaryformatter-obsoletion.md).

The V1 version of the Remoting protocol has been deprecated. Services should migrate to the V2_1 version of the Remoting protocol, which uses
DataContractSerializer for message serialization and allows using DataContractSerializer for exception serialization. Note that any protocol change in a
distributed system requires multi-stage rollouts where new version listeners are enabled on all nodes first, then clients are switched to the new version,
and finally old version listeners are removed.

## How to upgrade Remoting to V2_1
- [Upgrade server and client applications to remoting V2_1](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-reliable-services-communication-remoting#upgrade-from-remoting-v1-to-remoting-v2-interface-compatible)
- [Switch exception serialization from BinaryFormatter to DataContractSerializer](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-reliable-services-exception-serialization#enable-data-contract-serialization-for-remoting-exceptions)

## Timeline

### Service Fabric Runtime version 6, SDK version 3 (2017)
- Introduced V2 version of the Service Fabric remoting protocol.
  - The V2 protocol uses DataContractSerializer for message serialization.
  - BinaryFormatter is enabled as a fallback for exception serialization by default.

### Service Fabric Runtime version 7, SDK version 4 (2019)
- Added netstandard2.0 binaries for use in .NET (Core) applications
- V2 is the only protocol available for .NET (Core) applications

### Service Fabric Runtime version 11, SDK version 8 (2025)
- All Remoting V1 APIs are marked obsolete. Their usage produces build warnings.
- Version agnostic Remoting APIs throw InvalidOperationException when service assembly doesn't have a specific ServiceRemotingProviderAttribute or
  ActorRemotingProviderAttribute. This helps services to fail fast and rollback the upgrade when the Remoting protocol change may be unexpected.
- The V2_1 protocol is used by default.
- BinaryFormatter is not enabled as a fallback for exception serialization by default.

### Service Fabric Runtime version 12, SDK version 9 (2026)
- All Remoting V1 APIs are removed.
- Exception serialization no longer supports the BinaryFormatter fallback.
