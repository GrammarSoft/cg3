#!/usr/bin/perl
use strict;
use warnings;
use Cwd qw(realpath);
use FileHandle;
use IPC::Open2;

my ($bindir, $sep) = $0 =~ /^(.*)(\\|\/).*/;
$bindir = realpath $bindir;
chdir $bindir or die("Error: Could not change directory to $bindir !");

my $bpath = $ARGV[0];
my $binary = $bpath."cg-proc";
my $compiler = $bpath."cg-comp";

`"$compiler" grammar.cg3 grammar.bin  >stdout.txt 2>stderr.txt`;

my $input = do {
  local $/;
  open my $fh, '<', 'input.txt' or die "Couldn't open input.txt for reading: $!\n";
  <$fh>;
};

# Server:
my $pid = open2(*Reader, *Writer, "$binary", "-z", "-d", "grammar.bin");

# Client:
my $tries = 10;
my $timeout = 2;
eval {
  local $SIG{ALRM} = sub { die "timed out"; };
  alarm($timeout);
  do {
    print Writer $input;
    do {
      local $/ = "\0";
      my $got = <Reader> . "\n"; # input/expected have a final newline
      open my $out, '>', "output.txt" or die "Couldn't open output.txt for writing: $!\n";
      print $out $got;

      `diff -a -B expected.txt output.txt >diff.txt`;
      if (-s "diff.txt") {
        last;
      }
      else {
        $tries--;
      }
    };
  } while ($tries > 0);
  alarm(0);
};

if (my $e = $@) {
  print STDERR "Fail: $e\n";
}
elsif ($tries != 0) {
  print STDERR "Fail.\n";
}
else {
  print STDERR "Success.\n";
}
