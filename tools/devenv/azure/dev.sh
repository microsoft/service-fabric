#!/bin/bash
set -e
CDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd `pwd` > /dev/null
cd $CDIR

# Load environmental variables from the conf files.
source $CDIR/resources/load_settings.sh

get_ssh_login()
{
    SSH_USER=`az group deployment show -g $OSSF_RG -n azuredeploy  --query properties.outputs.sshUser.value`
    # remove double quotes.
    SSH_USER="${SSH_USER//\"}"
    SSH_HOST=`az group deployment show -g $OSSF_RG -n azuredeploy  --query properties.outputs.sshHost.value`
    SSH_HOST="${SSH_HOST//\"}"
}

deploy()
{
    echo "Creating resource group called $OSSF_RG in the $OSSF_AZREGION and deploying a VM called $OSSF_VMNAME with SKU $OSSF_VMSKU"
    echo 
    if [ `az group exists -n $OSSF_RG` == "false" ]
    then
        read -p "Are you sure you want to create this resource? Charges to your azure subscription will apply. (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Creating azure vm and supplying your public key from ~/.ssh/id_rsa.pub"
            az group create --name $OSSF_RG --location $OSSF_AZREGION
            az group deployment create --resource-group $OSSF_RG \
                --template-file resources/azuredeploy.json \
                --parameters \
                    vmSize="$OSSF_VMSKU" \
                    sshKeyData="`cat ~/.ssh/id_rsa.pub`" \
                    vmName="$OSSF_VMNAME"
            get_ssh_login
            scp $CDIR/resources/pull_git_and_docker_on_vm.sh $SSH_USER@$SSH_HOST:~/
        fi
    fi
    get_ssh_login
    ssh -t $SSH_USER@$SSH_HOST './pull_git_and_docker_on_vm.sh'
    echo "Setup complete... You can login and check things out by executing the following:"
    echo "dev.sh connect"
        
    exit

}

check_for_rg_and_create()
{
    [ `az group exists -n $OSSF_RG` == "false" ] && echo "Resource group does not exist.. Creating new one." && deploy
    return 0 
}

connectvm()
{
    check_for_rg_and_create

    get_ssh_login    
    echo "Connecting to remote host=$SSH_HOST under user=$SSH_USER..."
    ssh -t $SSH_USER@$SSH_HOST "cd /mnt/; bash"
    exit
}

deletevm()
{
    [ `az group exists -n $OSSF_RG` == "false" ] && echo "Resource group does not exist.. Nothing to delete."
    echo "Deleting resource group \"$OSSF_RG\""
    # az cli will prompt with an "Are you sure" message.
    az group delete -n "$OSSF_RG"
}
print_help_text()
{
    cat <<EOF
Usage:
dev.sh [command] 
    h, help: Displays this help text
    d, deploy: Deploys an azure VM with docker, clones the repository and pulls the docker image
    c, connect: Connects you to the azure VM
    cc, connect_container: Drops you into an interactive shell inside a docker container on the VM
    delete : Deletes the associated resource group and VM from your azure subscription
This tool is for setting up and managing a development environment in azure.

Default configuration file is found in settings.default.conf.

You can set user defined configurations by creating and overriding variables 
in a settings.conf  file.

Configuration File Options 
    OSSF_RG         The name for the resource group containing your VM
    OSSF_AZREGION   The region to deploy your VM in
    OSSF_VMSKU      The Azure VM identifier for your VM with spaces replaced with underscores. 
                    See https://azure.microsoft.com/en-us/pricing/details/virtual-machines/linux/#Linux for options
    OSSF_VMNAME     The name for your VM.            
EOF
}

connect_container()
{
    check_for_rg_and_create

    echo "Dropping into an interactive prompt inside a docker container on the azure VM..."
    get_ssh_login
    ssh -t $SSH_USER@$SSH_HOST "/mnt/service-fabric/connect.sh"
    exit    
}

if [ "$1" == "h" ] || [ "$1" == "help" ]; then
    print_help_text
elif [ "$1" == "d" ] ||[ "$1" == "deploy" ]; then
    deploy
elif [ "$1" == "c" ] || [ "$1" == "connect" ]; then
    connectvm
elif [ "$1" == "cc" ] || [ "$1" == "connect" ]; then
    connect_container
elif [ "$1" == "delete" ]; then
    deletevm
else
    print_help_text
fi

popd > /dev/null