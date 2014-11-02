#
# Copyright(c) 2014 - Krister Hedfors
#

unset LD_PRELOAD

#gcc -Wall -fPIC -c -o liblfish.o liblfish.c
#gcc -shared -fPIC -Wl,-soname -Wl,liblfish.so -o liblfish.so liblfish.o

GCC_FLAGS="-DLFISH_ROOT=\"$PWD\""

echo gcc -Wall -ggdb -shared -fPIC -o liblfish.so liblfish.c -ldl -lpython2.7 -lm -L/usr/lib/python2.7/config
if ! gcc -Wall -ggdb -shared -fPIC -o liblfish.so liblfish.c -ldl -lpython2.7 -lm -L/usr/lib/python2.7/config $GCC_FLAGS
then
    echo ">>>>>>>>>>>>>>>>>" COMPULATION ERRORS
else
    gcc -Wall -ggdb -shared -fPIC -o liblfish.C liblfish.c -E

    echo export LD_PRELOAD=$PWD/liblfish.so
    export LD_PRELOAD=$PWD/liblfish.so

    echo
    echo ">>>>>>>>>>>>>>>>>" RUNNING TESTS
    echo

    echo "LFISH_ABSPATH='${PWD}/lfish.py'" > config.py

    if [ ! -f testopen ]
    then
        gcc -o testopen testopen.c -ggdb
    fi
    echo ./testopen /proc/net/tcp
    ./testopen /proc/net/tcp


    if [ -f core ]
    then
        unset LD_PRELOAD
        echo
        echo ">>>>>>>>>>>>>>>>>" CORE DUMPED, press enter to debug
        echo
        read
        echo -en "bt\n" | gdb -x /dev/stdin -c core testopen | less
        rm -f core
    fi

    echo -e \\n\\n\\n
    echo "  * LD_PRELOAD is now set to: LD_PRELOAD=$LD_PRELOAD"
    echo "  * which means that any                                           *"
    echo "  * command executed from this shell will be subject to your hooks.*"
    echo "  *                                                                *"
    echo "  * Try for instance:                                              *"
    echo "  * $ ./testopen /proc/net/tcp                                     *"
    echo "  *                                                                *"
    echo "  * Disable hooks by unsetting LD_PRELOAD:                         *"
    echo "  * $ unset LD_PRELOAD                                             *"
    echo "  *                                                                *"
    echo "  * Enjoy! /Krister                                                *"
    echo
fi

#unset LFISH_DEBUG
#unset LD_PRELOAD
