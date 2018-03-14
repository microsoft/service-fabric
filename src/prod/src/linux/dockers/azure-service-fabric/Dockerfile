# Service Fabric Dockerfile
FROM ubuntu:16.04

RUN echo "build ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ADD FabricDrop /home/FabricDrop
ADD ClusterDeployer /home/ClusterDeployer
ADD dockers/azure-service-fabric/ClusterManifest.SingleMachineFSS.xml /home/ClusterDeployer
ADD dockers/azure-service-fabric/setup.sh /home/ClusterDeployer
ADD dockers/azure-service-fabric/run.sh /home/ClusterDeployer

EXPOSE 19080
WORKDIR /home/ClusterDeployer
