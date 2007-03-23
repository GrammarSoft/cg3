#!/bin/bash
. combine.sh
xsltproc ./docbook-xsl/html/onechunk.xsl manual-combined.xml
