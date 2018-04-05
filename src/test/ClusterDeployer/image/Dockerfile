# Service Fabric Dockerfile
FROM ubuntu:16.04

RUN echo "build ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ADD dockers/cluster-deployer/setup.sh /
RUN /setup.sh
ADD ClusterDeployer /home/ClusterDeployer
ADD dockers/cluster-deployer/ClusterManifest.SingleMachineFSS.xml /home/ClusterDeployer
ADD dockers/cluster-deployer/run.sh /home/ClusterDeployer

EXPOSE 19080
WORKDIR /home/ClusterDeployer
