mkdir /tmp/clustertest
pushd /tmp/clustertest
echo "Cloning sample application"
git clone --depth=1 https://github.com/Azure-Samples/service-fabric-dotnet-core-getting-started code
cd code/Services/CounterService
echo "Building application"
./build.sh > /dev/null
echo "done"
echo "Installing application"
sfctl cluster select --endpoint http://localhost:19080
./install.sh
if [[ $? != 0 ]]; then
  echo "Unable to install application to cluster. Test failed."
  popd
  exit 1
fi
echo "done"

popd
rm -rf /tmp/clustertest

echo "Attempting to connect to the application on http://localhost:31002"
Retries=0
# The application takes a few second to come up.
# Waiting 2 minutes 
MaxWaitTimeSeconds=120
RetryIntervalSeconds=10
MaxRetries=$((MaxWaitTimeSeconds/RetryIntervalSeconds))
while [ $Retries -lt $MaxRetries ]; do
  curl -s localhost:31002 > /dev/null
  if [[ $? == 0 ]]; then
    echo "Test passed."
    exit 0
  fi
  Retries=$((Retries+1))
  sleep $RetryIntervalSeconds
done

echo "Unable to connect to the application. Test failed"
exit 1
