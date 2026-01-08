# Client EKU Removal Notice

Dear Service Fabric customers,

The CA Browser Forum has made a decision to remove ClientAuth EKUs in publicly trusted TLS certificates. This change reflects the expectation of these certificate's usage only as public web server certificates. There is a lot of information available publicly and online about these upcoming changes.[1]

A Service Fabric-specific troubleshooting guide is also available. [2]

In many configurations of Service Fabric, Service Fabric requires a Bring-Your-Own certificate called the Cluster Certificate. The cluster certificate is used extensively across Service Fabric, including the fundamental peer-to-peer trust which is the basis of the Federation layer of Service Fabric, as well as used by the Service Fabric system services to talk among themselves, and to present to you as a client when you connect to the Service Fabric management plane. Service Fabric makes extensive use of mTLS for these connections, which is a standard practice for peer trust. Because of Service Fabric's usage of these certificates as mTLS certificates, Service Fabric requires the cluster certificate to include both client, and server EKUs. Because dual use certificates have been the historical standard, customers building Service Fabric X509 clusters have likely used publicly issued TLS certificates as the cluster certificate because of the ease in acquiring, managing, and renewing them. They have also been historically suitable for usage in SF. Guidance from SF may have not clearly stated this requirement for dual-use EKUs, likewise because they were the standard.

Currently CAs are planning to stop renewing certificates with client EKU between May and October of 2026.[3] If the cluster certificate is renewed to a certificate without a client EKU, some scenarios in Service Fabric may break. Specifically, it is known that Service Fabric Managed Identity may break, but the platform generally cannot guarantee stability with Server EKU-only certificates.

A common question would be why Service Fabric is not resilient to this change. Unfortunately, it is not a technical limitation to the platform's resilience, but that Service Fabric must respect the key usages of the certificates you are bringing to it, to guarantee platform correctness and security. We highly recommend reviewing your other service-to-service, mTLS, and infrastructural usage of certificates outside of Service Fabric, as they may also be impacted by this change.

## How to action on this information?

In short, going forward, your Service Fabric cluster certificate and client certificates must retain their client EKUs as they continue to renew periodically.

1. Understand if you are using X509 configured clusters (rather than e.g. Windows auth configured clusters)
2. Understand if you are using public CA chained certificates (rather than self-signed, or private PKI certs that you or your company control the configuration of)
3. If they are publicly chained certificates, please reach out to your CA to understand if your certificates will be renewed without client EKU, and what migration possibilities are possible.
4. If you can acquire new, dual-use certificates with the same Common Name as you have configured your cluster for [4] no action may be required.
5. It may be necessary to make changes to your Key Vault, or certificate provisioning methodologies, including changes to your Azure VMSS or compute in order to provision new certificates to the nodes of your Service Fabric cluster.
6. It may be necessary to make changes to your cluster's certificate configuration if your migration involves a change in Certificate common name or thumbprint.

Thanks,  
Service Fabric team

## References

[1] The links are shared as reference, but your Certificate Authority is the best source of information on the changes.
- [Sunsetting the ClientAuth EKU: What, Why, and How to Prepare for the Change | RSAC Conference](https://www.rsaconference.com/library/blog/sunsetting-the-clientauth-eku-what-why-and-how-to-prepare-for-the-change)
- [Client Authentication EKU Phased Out - KeyTalk](https://keytalk.com/news/client-authentication-eku-phased-out)

[2] [Add TSG for Client Authentication EKU removal impact by jagilber · Pull Request #320 · Azure/Service-Fabric-Troubleshooting-Guides](https://github.com/Azure/Service-Fabric-Troubleshooting-Guides/pull/320/files)

[3] Digicert's schedule is available below:  
[Sunsetting the client authentication EKU from DigiCert public TLS certificates](https://knowledge.digicert.com/alerts/sunsetting-client-authentication-eku-from-digicert-public-tls-certificates)

[4] [X.509 Certificate-based Authentication in a Service Fabric Cluster - Azure Service Fabric | Microsoft Learn](https://learn.microsoft.com/en-us/azure/service-fabric/cluster-security-certificates#validation-rules)
