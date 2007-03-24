#!/bin/bash
. combine.sh
xsltproc ./docbook-xsl/html/chunk-chapters.xsl manual-combined.xml
