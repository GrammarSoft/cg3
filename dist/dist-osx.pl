#!/usr/bin/perl
use strict;
use warnings;

my $dname = `dirname "$0"`;
chomp($dname);
chdir($dname.'/..');

print STDERR `svn up`;
print STDERR `make -j5`;
my $revision = `svnversion -n`;
$revision =~ s/^([0-9]+).*/$1/g;

print STDERR `rm -rfv /tmp/vislcg3-0.9.8.$revision-osx* 2>&1`;
mkdir('/tmp/vislcg3-0.9.8.'.$revision.'-osx');
chdir('/tmp/vislcg3-0.9.8.'.$revision.'-osx');
mkdir('lib');
mkdir('bin');
print STDERR `cp -av $dname/../src/vislcg3 ./bin/ 2>&1`;
print STDERR `cp -av $dname/../src/cg-comp ./bin/ 2>&1`;
print STDERR `cp -av $dname/../src/cg-proc ./bin/ 2>&1`;
print STDERR `cp -av $dname/../src/cg-conv ./bin/ 2>&1`;
foreach my $bin (('vislcg3','cg-comp','cg-proc','cg-conv')) {
   print STDERR `ln -sv ./bin/$bin $bin`;
   print STDERR `install_name_tool -change libicuuc.51.dylib \@executable_path/../lib/libicuuc.51.dylib ./bin/$bin`;
   print STDERR `install_name_tool -change libicuio.51.dylib \@executable_path/../lib/libicuio.51.dylib ./bin/$bin`;
   print STDERR `install_name_tool -change libicui18n.51.dylib \@executable_path/../lib/libicui18n.51.dylib ./bin/$bin`;
}
print STDERR `cp -av $dname/../scripts/cg3-autobin.pl ./bin/ 2>&1`;
print STDERR `cp -av /usr/local/lib/libicu*.dylib ./lib/ 2>&1`;

chdir('/tmp');
print STDERR `tar -zcvf 'vislcg3-0.9.8.$revision-osx.tar.gz' 'vislcg3-0.9.8.$revision-osx' 2>&1`;
print STDERR `ls -l '/tmp/vislcg3-0.9.8.$revision-osx.tar.gz'`;
print "/tmp/vislcg3-0.9.8.$revision-osx\n";
