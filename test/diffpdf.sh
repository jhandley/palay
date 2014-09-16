#!/bin/sh

set -e

SCRIPT_NAME=$0

if [ "$#" -ne 2 ]; then
    echo "$SCRIPT_NAME <pdf1> <pdf2>"
    exit 1
fi

command -v pdftoppm >/dev/null 2>&1 || { echo "$SCRIPT_NAME: pdftoppm required" >&2; exit 1; }

PDF1=$1
PDF2=$2
PDFTOPPM="pdftoppm -aa no -aaVector no"

rm -fr _pdf1 && mkdir _pdf1
rm -fr _pdf2 && mkdir _pdf2

$PDFTOPPM $PDF1 _pdf1/
$PDFTOPPM $PDF2 _pdf2/

PDF1_PAGES=$(ls _pdf1/*.ppm)
PDF2_PAGES=$(ls _pdf2/*.ppm)
PDF1_PAGE_COUNT=$(echo $PDF1_PAGES | wc -w)
PDF2_PAGE_COUNT=$(echo $PDF2_PAGES | wc -w)

if [ $PDF1_PAGE_COUNT -ne $PDF2_PAGE_COUNT ]; then
    echo "$SCRIPT_NAME: different number of pages"
    rm -fr _pdf*
    exit 1
fi

if ! diff _pdf1 _pdf2 >/dev/null; then
    echo "$SCRIPT_NAME: found difference between page renderings"
    rm -fr _pdf*
    exit 1
fi

rm -fr _pdf*
exit 0
