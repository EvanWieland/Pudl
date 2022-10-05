##!/bin/bash
#
##./build/pudl "$@"
#./debug/pudl "$@"

#!/bin/bash
if [ "$#" -eq "0" ]; then
  # src="example/main.t"
  src="examples/main.t"
  out="debug/out.3ac"
elif [ "$#" -eq "1" ]; then
  src=$1
  out="debug/out.3ac"
else
  src=$1
  out=$2
fi

echo $src
echo $out

echo "BUILD:"
debug/pudl $src $out
debug/pudl $src "debug/opt.3ac" -Oall
echo
echo
echo "RUN:"
lli $out