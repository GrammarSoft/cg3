echo "- aclocal."               && \
aclocal -I m4                   && \
echo "- autoconf."              && \
autoconf                        && \
echo "- automake."              && \
automake --add-missing --gnu    && \
echo                            && \
./configure "$@"                && exit 0
