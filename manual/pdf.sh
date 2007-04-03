#!/bin/bash
. xsl-fo.sh
java -jar /Programmer/Java/Custom/fop.jar -dpi 150 -fo cg3.fo -pdf cg3.pdf
