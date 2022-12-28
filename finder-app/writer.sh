if [ $# != 2 ]
then
	echo "Invalid parameters"
	exit 1
fi

writefile=$1
writestr=$2

directory=$(dirname "$writefile")

if [ ! -d $directory ]
then
	mkdir $directory
fi

echo $writestr > $writefile
