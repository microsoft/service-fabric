sudo mkdir -p /mnt/service-fabric
sudo chown -R $USER /mnt/service-fabric
git clone "https://github.com/Microsoft/service-fabric.git" /mnt/service-fabric
cd /mnt/service-fabric
docker pull Microsoft/service-fabric-build-ubuntu
