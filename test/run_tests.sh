#!/bin/bash

SCRIPT_NAME=$0
TEST_DIR=$(dirname $(readlink -f $0))
TESTS=$*

red='\E[31m'
green='\E[32m'
normal='\E[0m'

PALAY=$TEST_DIR/../palay/palay
COMPAREPDF=$TEST_DIR/diffpdf.sh

command -v $COMPAREPDF >/dev/null 2>&1 || { echo "$SCRIPT_NAME: comparepdf required. Install by 'sudo apt-get install comparepdf' or similar" >&2; exit 1; }
command -v $PALAY_BIN >/dev/null 2>&1 || { echo "$SCRIPT_NAME: palay not found. Build it first." >&2; exit 1; }

export PALAY
export COMPAREPDF

# point to libpalay.so
export LD_LIBRARY_PATH=$TEST_DIR/../libpalay

FAILED=false
run() {
    TEST=$1
    echo Running $TEST...
    cd $TEST_DIR/$TEST

    # Clean up any previous results
    rm -f actual*
    if ! sh -e test; then
        echo -e "${red}$TEST failed!$normal"
        FAILED=true
    else
        # Clean up results since it passed
        rm -f actual*
    fi
}

cd $TEST_DIR
[ "$TESTS" == "" ] && TESTS=$(find . ! -path . -type d | sort)
for TEST in $TESTS; do
    run $TEST
done

if $FAILED; then
    echo -e "${red}One or more tests failed!$normal"
    exit 1
else
    echo -e "${green}All tests passed!$normal"
    exit 0
fi
