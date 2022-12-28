if [ ! -d $1 ] 
then
	echo "$1 is not a valid directory!"
	exit 1
fi

if [ $# != 2 ] 
then
	exit 1
fi

filesdir=$1
searchstr=$2

num_of_files=$(find $filesdir -type f | wc -l)
num_of_matches=$(grep $searchstr $filesdir/* | wc -l)

echo  "The number of files are $num_of_files and the number of matching lines are $num_of_matches"


