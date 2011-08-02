#!/usr/bin/perl
use strict;
use warnings;

my $dname = `dirname "$0"`;
chdir($dname);

print `svn up --ignore-externals`;
my $revision = `svnversion -n`;
$revision =~ s/^([0-9]+).*/$1/g;

print `rm -rfv '/tmp/vislcg3-0.9.7.$revision-osx*' 2>&1`;
mkdir('/tmp/vislcg3-0.9.7.'.$revision.'-osx');
chdir('/tmp/vislcg3-0.9.7.'.$revision.'-osx');
mkdir('lib');
mkdir('bin');
print `cp -av "$dname/osx/*" ./ 2>&1`;
print `cp -av /usr/local/bin/vislcg3 ./bin/ 2>&1`;
print `cp -av /usr/local/bin/cg-* ./bin/ 2>&1`;
print `cp -av /usr/local/bin/cg3* ./bin/ 2>&1`;
print `cp -av /usr/local/lib/libicu*.dylib ./lib/ 2>&1`;

chdir('/tmp');
print `tar -zcvf 'vislcg3-0.9.7.$revision-osx.tar.gz' 'vislcg3-0.9.7.$revision-osx' 2>&1`;
print `ls -l '/tmp/vislcg3-0.9.7.$revision-osx.tar.gz'`;
