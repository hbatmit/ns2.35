if [ $# -ne 2 ]; then
	echo "migrate.sh <source path (to kernel directory) > <destination path (to tcp/linux/src-<version#> directory)>"
	echo "    This script goes through the net/ipv4/tcp_*.c directory to find all the congestion control module source code and copy them"
	echo " to the destination directory. It also changes the header files of the source files so that it can be hooked into NS-2 TCP-Linux"
	exit 1;
fi
src=$1
dst=$2
mkdir -p $dst
cp src/* $dst/
file_list=`ls $src/net/ipv4/tcp_*.c`
for i in $file_list 
do
	is_cc=`cat $i | grep -c "tcp_register_congestion_control"`
	if [ $is_cc -gt 0 ]; then
		dstname=`basename $i`
		echo "/* Modified Linux module source code from $1 */" > $dst/$dstname
		echo "#define NS_PROTOCOL \"$dstname\"" >> $dst/$dstname
		echo "#include \"../ns-linux-c.h\"" >> $dst/$dstname
		echo "#include \"../ns-linux-util.h\"" >> $dst/$dstname
		cat $i | grep -v ^"#include <" >> $dst/$dstname
		echo "#undef NS_PROTOCOL" >> $dst/$dstname

	fi
done
# check for header files too
cp $src/net/ipv4/tcp_*.h $dst/
rm src
ln -s $dst src
