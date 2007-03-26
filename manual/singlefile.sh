#!/bin/bash
. combine.sh
xsltproc -o index.html ./docbook-xsl/html/docbook.xsl manual-combined.xml
