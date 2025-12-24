# Service Fabric WCF Remoting Binary Serialization Deprecation Strategy
Until Service Fabric SDK version `8`, Service Fabric WCF Remoting was using binary 
serialization for exception propagation. As binary serialization is unsafe and is 
removed from newer .NET versions, WCF Remoting has switched to use Data Contract 
Serialization (DCS), and since SDK version `9`, binary serialization is removed 
from WCF Remoting.

SDK version `8.1.0` enables seamless migration of active services from binary serialization
of exceptions to DCS. Listner version `8.1.0` can send exceptions using either binary serialization or DCS, depending on provided `ExceptionSerializationTechnique` (`BinaryFormatter` for binary serialization or `Default` for DCS). On the other side, client version `8.1.0` will accept exceptions serialized by any of these techniques.

## How to upgrade to WCF Remoting to DCS
Migration of services using WCF Remoting to DCS can be done by following steps:

1. Upgrade both client and listener service (in any order) to version `8.1.0`. Listener should
be configured to use binary serialization:

```csharp
WcfRemotingListenerSettings settings = new WcfRemotingListenerSettings();
settings.ExceptionSerializationTechnique = FabricTransportRemotingListenerSettings.ExceptionSerialization.BinaryFormatter;

var ServiceInstanceListener = new ServiceInstanceListener(context => new WcfServiceRemotingListener(context, this, null, null, null, false, null, settings));
```

Client in version `8.1.0` is capable of accepting exception serialized using either binary serialization or DCS.

---
**NOTE**

Due to a known issue, this settings has to be applied programatically. Config values provided 
in .xml configuration are ignored.

---

2. Switch listener service to use DCS:

```csharp
WcfRemotingListenerSettings settings = new WcfRemotingListenerSettings();
settings.ExceptionSerializationTechnique = FabricTransportRemotingListenerSettings.ExceptionSerialization.Default;

var ServiceInstanceListener = new ServiceInstanceListener(context => new WcfServiceRemotingListener(context, this, null, null, null, false, null, settings));
```

After this, both client and listener can be upgraded to newer SDK versions in any order 
without any further configuration changes.