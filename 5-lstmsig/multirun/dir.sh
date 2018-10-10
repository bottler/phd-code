source defs.sh
options=("list groups" "list resources" "list cli resources" "list vms" "list cli vms" "control vm" "images" "async tasks" "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "list groups")
            azure group list --subscription $SUBSCR 
            ;;
        "list resources")
            #azure resource list --subscription $SUBSCR
            #sed is removing the color info from the output, which makes the data friendlier to cut.
            #the $ on the sed string causes \e to be a literal escape character, and doesn't effect the \+
            #you can type a literal escape in bash with ctrl-v ctrl-[ but not including one in this file is nicer
            azure resource list --subscription $SUBSCR | sed $'s/\e[^a-zA-Z]\+[a-zA-Z]//g' | grep "^data"|cut --complement -c 1-129
            ;;
        "list cli resources")
            azure resource list --subscription $SUBSCR --resource-group $RG | sed $'s/\e[^a-zA-Z]\+[a-zA-Z]//g' | grep "^data"|cut --complement -c 1-129
            ;;
        "list vms")
            azure vm list --subscription $SUBSCR
            ;;
        "list cli vms")
            azure vm list --subscription $SUBSCR --resource-group $RG
            ;;
        "async tasks")
            select opt2 in "start" "deallocate"
            do
                read -p "id? " ID
                azure vm $opt2 --subscription $SUBSCR $RG $vmroot$ID &
            done
            break #quit script to see outputs
            ;;
        "control vm")
            opts2=("show" "create" "delete" "deallocate" "start" "ssh" "sendfiles" "cmd" "difffiles" "send1file" "diff1file" "pullresults" "get-instance-view" "Quit")
            read -p "id? " ID
            if [[ ! -z "$ID" ]]; then
                PS3="$ID #?"
                select opt2 in "${opts2[@]}"
                do
                    case $opt2 in                       
                        "show"|"start"|"get-instance-view")
                            azure vm $opt2 --subscription $SUBSCR $RG $vmroot$ID
                            ;;
                        "deallocate")
                            time azure vm $opt2 --subscription $SUBSCR $RG $vmroot$ID
                            ;;
                        "delete")
                            read -r -p "Are you sure you want to delete $ID? [y/N] " response
                            response=${response,,}    # tolower
                            if [[ "$response" =~ ^(yes|y)$ ]]; then                              
                                azure vm delete --subscription $SUBSCR $RG $vmroot$ID
                            fi
                            read -r -p "Are you sure you want to delete the network interface $nicroot$ID [y/N] " response
                            response=${response,,}    # tolower
                            if [[ "$response" =~ ^(yes|y)$ ]]; then                              
                                azure network nic delete --subscription $SUBSCR --resource-group $RG --name $nicroot$ID
                            fi
                            read -r -p "Are you sure you want to delete the ip $ipnameroot$ID? [y/N] " response
                            response=${response,,}    # tolower
                            if [[ "$response" =~ ^(yes|y)$ ]]; then                              
                                azure network public-ip delete --subscription $SUBSCR --resource-group $RG --name $ipnameroot$ID
                            fi
                            ;;
                        "ssh")
                            if [[ ! -z "$STY" ]]; then #tell screen to label the window
                                echo -ne "\033k$ID\033\\"
                            fi
                            ssh -i azure_rsa fred@$hostnameroot$ID.$region.cloudapp.azure.com
                            if [[ ! -z "$STY" ]]; then
                                echo -ne "\033kbash\033\\"
                            fi
                            ;;
                        "cmd")
                            read -p "cmd? " CMD
                            if [[ ! -z "$CMD" ]]; then 
                               ssh -i azure_rsa fred@$hostnameroot$ID.$region.cloudapp.azure.com $CMD
                            fi
                            ;;
                        "create")
                            #azure vm sizes -v --location northeurope|grep DS
                            #DS2 and DS11 are both 2 core, latter not much dearer and 14Gb not 7Gb
                            select size in Standard_DS12 Standard_DS2 Standard_DS11 Standard_NC6
                            do
                                read -r -p "Need to create the ip $ipnameroot$ID? [y/N] " response
                                response=${response,,}    # tolower
                                if [[ "$response" =~ ^(yes|y)$ ]]; then                              
                                    azure network public-ip create --subscription $SUBSCR --resource-group $RG --location $region --name $ipnameroot$ID --domain-name-label $hostnameroot$ID --idle-timeout 4
                                fi
                                read -r -p "Need to create the network interface $nicroot$ID? [y/N] " response
                                response=${response,,}    # tolower
                                if [[ "$response" =~ ^(yes|y)$ ]]; then                              
                                    azure network nic create --resource-group $RG --location $region --name $nicroot$ID --subnet-vnet-name $vnet --subnet-name $subnet --subscription $SUBSCR --public-ip-name $ipnameroot$ID
                                fi
                                azure vm create --nic-name $nicroot$ID --subscription $SUBSCR -M azure_rsa.pub -Q $QQ -z $size -o $SA -u fred --disable-boot-diagnostics -R vhds -S $subnet -F $vnet --resource-group $RG --name $vmroot$ID --location $region --os-type Linux
                                ssh-keygen -R $hostnameroot$ID.$region.cloudapp.azure.com #forget any stored SSH keys, otherwise nasty message next time you create a vm with the same name
                                break
                            done
                            ;;
                        "sendfiles")
                            rsync -avz -e "ssh -i azure_rsa" lstm/*.py fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm
                            rsync -avz -e "ssh -i azure_rsa" lstm/models/blank.txt fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm/models/
                            rsync -avz -e "ssh -i azure_rsa" inside.sh fred@$hostnameroot$ID.$region.cloudapp.azure.com:
                            rsync -avz -e "ssh -i azure_rsa" screenrc fred@$hostnameroot$ID.$region.cloudapp.azure.com:.screenrc
                            ;;
                        "pullresults")
                            DA=`date +%Y%m%d-%H%M%S`
                            rsync -avz -e "ssh -i azure_rsa" fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm/results.sqlite pulled/results_$ID.$DA.sqlite
                            rsync -avz -e "ssh -i azure_rsa" fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm/models pulled/models/$ID.`date +%Y%m%d-%H%M%S`
                            ;;
                        "send1file")
                            read -e -p "file?" FILE #with completion
                            read -p "Are you sure? " -n 1 -r
                            echo  # move to a new line
                            if [[ $REPLY =~ ^[Yy]$ ]]; then
                               rsync -avz -e "ssh -i azure_rsa" $FILE fred@$hostnameroot$ID.$region.cloudapp.azure.com:$FILE
                            fi
                            ;;
                        "difffiles")
                            rsync -avn -e "ssh -i azure_rsa" lstm/*.py fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm
                            echo
                            rsync -avun -e "ssh -i azure_rsa" lstm/*.py fred@$hostnameroot$ID.$region.cloudapp.azure.com:lstm
                            ;;
                        "diff1file")
                            FILE=lstm/results.py
                            ssh -i azure_rsa fred@$hostnameroot$ID.$region.cloudapp.azure.com cat $FILE |colordiff - $FILE
                            ;;
                        *)
                            break
                            ;;
                    esac
                done
                PS3="#?"
            fi
            ;;
        "images")
            azure vm image list northeurope canonical ubuntuserver|grep 16.04-LTS
            #azure vm image list northeurope OpenLogic centos
            echo
            echo $QQ
            ;;
        "Quit")
            break
            ;;
        *) echo invalid option;;
    esac
done
