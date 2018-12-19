#!/bin/sh

# syntax is -o to redirect output, -s to strip, -d to run in debug, -s to skip media, -a to skip ant install
#--pvdhost=XX.XX.XX.XX --pvdport=XX --pvdtimeout=XX --nonVizPvd

debug=0
strip=0
skipMedia=0
skipAntInstall=0

pvd_host="127.0.0.1"
pvd_port="5425"
pvd_timeout=10
nonVizPvd=0

target=
	
cmdFile=../../media/PhysX/3.4/Samples/user/androidCmdLine.cfg
function saveArgs()
{
	:> $cmdFile
	echo -n "--pvdhost=$pvd_host --pvdport=$pvd_port --pvdtimeout=$pvd_timeout">>$cmdFile
	if test $nonVizPvd -eq 1
	then
		echo " --nonVizPvd">>$cmdFile
	else
		echo $'\n'>>$cmdFile
	fi
}

function parseArgs()
{
while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    VALUE=`echo $1 | awk -F= '{print $2}'`
	case $PARAM in
        -h|--help)
            exit;;
		debug) target=$PARAM;suffix=DEBUG;; 
		checked)target=$PARAM;suffix=CHECKED;; 
		profile)target=$PARAM;suffix=PROFILE;; 
		release)target=$PARAM;suffix=;; 
		-d|--d)
		 debug=1;;
		-s|--s)
			strip=1;;	
		-m|--m)
			skipMedia=1;;
		-a|--a)
			skipAntInstall=1;;	
		-nonVizPvd|--nonVizPvd)
			nonVizPvd=1;;
		-pvdhost|--pvdhost)
			if [ -z $VALUE ]
			then
				shift
				VALUE=$1
			fi
			pvd_host=$VALUE;;
		-pvdport|--pvdport)
			if [ -z $VALUE ]
			then
				shift
				VALUE=$1
			fi
			pvd_port=$VALUE;;
		-pvdtimeout|--pvdtimeout)
			if [ -z $VALUE ]
			then
				shift
				VALUE=$1
			fi
			pvd_timeout=$VALUE;;		
        *)
        echo "ERROR: unknown parameter \"$PARAM\""
        exit 1
        ;;
    esac
    shift
done

if [ -z $target ] 
then
	echo Must have a config!; exit 1
fi
}

