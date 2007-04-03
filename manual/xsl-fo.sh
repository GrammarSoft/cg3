#!/bin/bash
. combine.sh
xsltproc -o cg3.fo --stringparam ulink.show 0 --stringparam ulink.footnotes 1 --stringparam fop1.extensions 1 --stringparam page.margin.top 1cm --stringparam page.margin.inner 1cm --stringparam page.margin.outer 1cm --stringparam page.margin.bottom 1cm --stringparam paper.type A4 ./docbook-xsl/fo/docbook.xsl manual-combined.xml
