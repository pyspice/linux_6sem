#!bin/bash

if [ $# -lt 3 ]
then
	echo -e "backup.sh: usage error
usage: ./backup.sh ARGS
	ARGS:
	1. <directory_name> - set backup directory name to directory_name
	2. <archive_name> - set backup archive name to archive_name
	3. <extensions_name> - set necessary extensions to extensions_name(s)"
	exit
fi

EXTS=".*\.("$3
for i in "${@:4}"
do
	EXTS=$EXTS"|$i"
done
EXTS=$EXTS")$"

CUR_DIR=$(pwd)
cd

DIR_NAME=$CUR_DIR"/"$1
mkdir $DIR_NAME

for FNAME in $(find * -regextype posix-egrep -regex $EXTS -type f)
do
	NNAME=$(echo $FNAME | sed 's/\//./')
	cp $FNAME $DIR_NAME"/"$NNAME
done

cd $CUR_DIR

ARCH_NAME=$2".tar.gz"
tar -czf $ARCH_NAME $1

echo Done