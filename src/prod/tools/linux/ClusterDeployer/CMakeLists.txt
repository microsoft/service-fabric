install(
    FILES
    ClusterManifest.AAD.xml
    ClusterManifest.SingleMachine.xml
    ClusterManifest.SingleMachineFSS.xml
    ClusterManifest.SingleMachineFSS.X509.xml
    ClusterManifest.SingleMachine.FSS.US.xml
    InternalIPs.txt
    PublicIPs.txt
    SSHPorts.txt
    DESTINATION ${CLUSTER_DEPLOYER_DIR}
)

install(
    FILES ./ClusterDeployer.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${CLUSTER_DEPLOYER_DIR}
)

install(
    FILES ./TraceProcessor.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${CLUSTER_DEPLOYER_DIR}
)

install(
    FILES ./CloseSession.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${CLUSTER_DEPLOYER_DIR}
)
