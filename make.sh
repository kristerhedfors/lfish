unset LD_PRELOAD

#gcc -Wall -fPIC -c -o liblfish.o liblfish.c
#gcc -shared -fPIC -Wl,-soname -Wl,liblfish.so -o liblfish.so liblfish.o

echo gcc -Wall -ggdb -shared -fPIC -o liblfish.so liblfish.c -ldl -lpython2.7 -lm -L/usr/lib/python2.7/config
if ! gcc -Wall -ggdb -shared -fPIC -o liblfish.so liblfish.c -ldl -lpython2.7 -lm -L/usr/lib/python2.7/config
then
    echo ">>>>>>>>>>>>>>>>>" COMPULATION ERRORS
else
    gcc -Wall -ggdb -shared -fPIC -o liblfish.C liblfish.c -E

    echo export LD_PRELOAD=$PWD/liblfish.so
    export LD_PRELOAD=$PWD/liblfish.so

    echo
    echo ">>>>>>>>>>>>>>>>>" RUNNING TESTS
    echo

    if [ ! -f testopen ]
    then
        gcc -o testopen testopen.c -ggdb
    fi
    echo ./testopen /proc/net/tcp
    ./testopen /proc/net/tcp


    if [ -f core ]
    then
        echo
        echo ">>>>>>>>>>>>>>>>>" CORE DUMPED, press enter to debug
        echo
        read
        echo -en "bt\n" | gdb -x /dev/stdin -c core testopen | less
        rm -f core
    fi
fi

#unset LFISH_DEBUG
#unset LD_PRELOAD
