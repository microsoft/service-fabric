#!/bin/bash
PrintUsage()
{
cat<<EOF
A tool for managing the lifetime of a local Service Fabric dev cluster in a Docker container. 
cluster.sh <verb> [options]
  Verbs:
    start: Starts up the cluster in a Docker container.
      -b, --rebuild: Rebuilds the container on demand
      -r, --restart: Stops and restarts the container
      -p: add docker port mapping arguments
    stop: Stops the cluster
    connect: Connects to the container in a bash prompt
    command: Connects to the container and runs a command
      command <command to run>
    copy: Copies a file into the container
      copy <source file> <destination>
EOF
}

IMAGE_NAME="service-fabric-run-ubuntu"

BuildImage()
{
  if [ ! -d "../../../out/build.prod/FabricDrop" ]; then
    echo "../../../out/build.prod/FabricDrop not found. You need to build using runbuild.sh."
    exit
  fi

  pushd "../../../out/build.prod"
  sudo docker build -f ClusterDeployerTest/image/Dockerfile -t $IMAGE_NAME . 
  popd
}

CheckImageExists()
{
  Count=$(sudo docker image ls -a | grep $IMAGE_NAME | wc -l)
  if [ "$Count" = "0" ]; then
    return 1
  fi
  return 0
}

CheckContainerExists()
{
  Count=$(sudo docker ps -a | grep sfoneboxdev | wc -l)
  if [ "$Count" != "0" ]; then
    return 0 
  fi
  return 1
}

# Tries to hit the cluster endpoint, if successful then returns true. If not then it checks for the container.
# If the container exists then we are in a bad state and the script recommends a restart then exits.
CheckClusterRunning()
{
  curl -s localhost:19080 2>&1 > /dev/null 
  local Result=$?

  if [ "$Result" != "0" ]; then
    CheckContainerExists
    local Created=$?
    if [ $Created -eq 0 ]; then
      echo "The container is running, but the cluster is not. Try running cluster.sh start --restart to start the cluster."
      exit 3
    fi
    return 1
  fi
  return 0
}

VerifyClusterRunning()
{
  CheckClusterRunning
  local rc=$?
  if [[ $rc != 0 ]]; then
    echo "Error: The cluster is not running"
    exit 5
  fi
}

StopContainer()
{
  CheckContainerExists
  if [ "$?" = "0" ]; then
    sudo docker kill sfoneboxdev
  fi
  Retries=0
  while [ "$Retries" -lt 5 ]; do
    CheckContainerExists
    if [ "$?" != "0" ]; then
      return 0
    fi
    Retries=$((Retries+1))
    sleep 1
  done
  echo "Container stopped"
}

StartVerb()
{
  Interactive=false
  Rebuild=false
  Ports=""
  while (( "$#" )); do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
      PrintUsage
      exit -1
    elif [ "$1" == "-r" ] || [ "$1" == "--restart" ]; then
      StopContainer
    elif [ "$1" == "-b" ] || [ "$1" == "--rebuild" ]; then
      Rebuild=true
    elif [[ "$1" =~ ^-p* ]]; then
      Ports="$Ports $1"
    else
      echo "Unknown start option $1"
      PrintUsage
      exit -2
    fi
    shift
  done

  CheckClusterRunning
  if [ $? -eq 0 ]; then
    echo "The cluster is already running"
    exit 0
  fi

  CheckImageExists
  Exists=$?
  if [ "$Rebuild" = "true" ] || [ $Exists -ne 0 ]; then
    echo "Building image..."
    BuildImage
  fi

  sudo docker run $Ports \
    -p19080:19080 \
    --rm \
    -td \
    --name sfoneboxdev \
    -v $(pwd)/../../../out/build.prod/FabricDrop:/home/FabricDrop \
    $IMAGE_NAME \
    bash -c "./run.sh"

  #Poll to check if the cluster is active by hitting SFX endpoint
  # Wait up to 10 minutes, testing every 30 seconds. The cluster should come up in less than 5.,
  # We can lower this by moving setup into into the container
  echo "Waiting for cluster to come up..."
  Counter=0
  while [ $Counter -lt 20 ]; do
    curl -s localhost:19080 2>&1 > /dev/null
    Code=$?
    if [ $Code != 0 ]; then
      echo "..."
      Counter=$((Counter+1))
      sleep 30
    else
      echo "The cluster is up. You can access Service Fabric Explorer at http://localhost:19080"
      exit 0
    fi
  done

  echo "Failed to start the cluster."
  exit 1
}

StopVerb()
{
  StopContainer
  exit 0
}

ConnectVerb()
{
  VerifyClusterRunning
  sudo docker exec -it sfoneboxdev /bin/bash
  exit 0
}

CopyVerb()
{
  VerifyClusterRunning
  sudo docker cp $1 sfoneboxdev:$2
  exit 0 
}

CommandVerb()
{
  VerifyClusterRunning
  sudo docker exec -i sfoneboxdev $@
  exit $?
}

if [ $# -lt 1 ]; then
  PrintUsage
  exit -2
fi

case $1 in
  "start")
    shift
    StartVerb $@
    ;;
  "stop")
    shift
    StopVerb $@
    ;;
  "connect")
    shift
    ConnectVerb $@
    ;;
  "copy")
    shift
    CopyVerb $@
    ;;
  "command")
    shift
    CommandVerb $@
    ;;
  "-h")
    PrintUsage
    exit -1
    ;;
  *)
    echo "Unknown verb $1"
    PrintUsage
    exit -2
esac

exit 0
