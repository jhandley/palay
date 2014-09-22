#!/bin/sh

set -e

SCRIPT_NAME=$0

if [ "$#" -lt 2 ]; then
    echo "$SCRIPT_NAME [-w] <pdf1> <pdf2>"
    echo "  Pass -w to not fail if the rendering of the PDFs is different"
    exit 1
fi

command -v pdftoppm >/dev/null 2>&1 || { echo "$SCRIPT_NAME: pdftoppm required" >&2; exit 1; }

RENDER_WARNING=false
if [ "$1" = "-w" ]; then
    RENDER_WARNING=true
    shift
fi

PDF1=$1
PDF2=$2
PDFTOPPM="pdftoppm -aa no -aaVector no"

PDF1_PAGE_COUNT=$(pdfinfo $PDF1 | grep "Pages:")
PDF2_PAGE_COUNT=$(pdfinfo $PDF2 | grep "Pages:")

# Check the page count is the same
if [ "$PDF1_PAGE_COUNT" != "$PDF2_PAGE_COUNT" ]; then
    echo "$SCRIPT_NAME: different number of pages"
    echo "  pdf1 -> $PDF1_PAGE_COUNT"
    echo "  pdf2 -> $PDF2_PAGE_COUNT"
    exit 1
fi

# Check that the page size is the same
PDF1_PAGESIZE=$(pdfinfo $PDF1 | grep "Page size:")
PDF2_PAGESIZE=$(pdfinfo $PDF2 | grep "Page size:")
if [ "$PDF1_PAGESIZE" != "$PDF2_PAGESIZE" ]; then
    echo "$SCRIPT_NAME: Page sizes are different"
    echo "  pdf1 -> $PDF1_PAGESIZE"
    echo "  pdf2 -> $PDF2_PAGESIZE"
    exit 1
fi

# Create temporary folders since the rest of the comparisons
# require them.
rm -fr _pdf1 && mkdir _pdf1
rm -fr _pdf2 && mkdir _pdf2

# Check that the text in the PDFs is the same
pdftotext $PDF1 _pdf1/pdf1.txt
pdftotext $PDF2 _pdf2/pdf2.txt
if ! diff _pdf1/pdf1.txt _pdf2/pdf2.txt > /dev/null; then
    echo "$SCRIPT_NAME: found difference in the text in the PDFs"
    echo " See _pdf1/pdf1.txt and _pdf2/pdf2.txt"
    rm -fr _pdf*
    exit 1
fi
rm _pdf1/pdf1.txt _pdf2/pdf2.txt

# Check that the renderings of the PDFs are the same
$PDFTOPPM $PDF1 _pdf1/
$PDFTOPPM $PDF2 _pdf2/
if ! diff _pdf1 _pdf2 >/dev/null; then
    echo "$SCRIPT_NAME: found difference between page renderings"
    rm -fr _pdf*
    # Don't fail if render warnings are enabled
    $RENDER_WARNING || exit 1
fi

rm -fr _pdf*
exit 0
