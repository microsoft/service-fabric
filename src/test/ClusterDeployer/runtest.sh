#!/bin/bash

PrintUsage()
{
cat<<EOF
This is a basic test of a Service Fabric cluster in a Docker container. It 
starts a container, starts the cluster, and then downloads, builds, and installs
a sample application from github. If the endpoint of the application can be
hit via curl then the test is successful. 

Usage: ./runtest.sh
EOF
}

if [ "$#" -gt "0" ]; then
  if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    PrintUsage
    exit -1
  else
    echo "Unknown argument $1"
    exit 1
  fi
fi

StopCluster()
{
  ./cluster.sh stop
}

echo "Starting cluster..."
./cluster.sh start -p31002:31002
if [ ! $? ]; then
  echo "Failed to start the cluster. Test failed."
  exit 1
fi

./cluster.sh copy test.sh /test.sh
./cluster.sh command bash -c "/test.sh"
rc=$?
StopCluster
exit $?
