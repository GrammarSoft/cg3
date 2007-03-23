#!/bin/bash
. combine.sh
xsltproc -o manual.html /usr/local/docbook/html/docbook.xsl manual-combined.xml
