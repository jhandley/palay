#!/bin/bash

SCRIPT_NAME=$0
TEST_DIR=$(dirname $(readlink -f $0))

PALAY=$TEST_DIR/../palay/palay
COMPAREPDF=$TEST_DIR/diffpdf.sh

command -v $COMPAREPDF >/dev/null 2>&1 || { echo "$SCRIPT_NAME: comparepdf required. Install by 'sudo apt-get install comparepdf' or similar" >&2; exit 1; }
command -v $PALAY_BIN >/dev/null 2>&1 || { echo "$SCRIPT_NAME: palay not found. Build it first." >&2; exit 1; }

export PALAY
export COMPAREPDF

# point to libpalay.so
export LD_LIBRARY_PATH=$TEST_DIR/../libpalay

run() {
    TEST=$1
    echo Running $TEST...
    cd $TEST_DIR/$TEST

    # Clean up any previous results
    rm -f actual*
    if ! sh -e test; then
        echo $TEST failed!
	exit 1
    fi

    # Clean up results since everything passed
    rm -f actual*
}

cd $TEST_DIR
TESTS=$(find . ! -path . -type d | sort)
for TEST in $TESTS; do
    run $TEST
done

echo All tests passed!
exit 0

