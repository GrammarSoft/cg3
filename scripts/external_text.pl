#!/usr/bin/env perl
# -*- mode: cperl; indent-tabs-mode: nil; tab-width: 3; cperl-indent-level: 3; -*-
BEGIN {
   $| = 1;
};
use warnings;
use strict;
use utf8;
#use feature 'unicode_strings';

use IO::Handle;
autoflush STDOUT 1;
autoflush STDERR 1;

use FindBin;
use lib $FindBin::Bin.'/';
use CG3_External qw(check_protocol read_window write_window write_null_response);

#my @af_cmd = ('sed', '-u', 's/w/WXU/g; s/ma/am/g;');
my @af_cmd = ('cat'); # Change to something meaningful

use IPC::Run qw(start pump finish timeout);
my $af_in;
my $af_out;
my $af_err;
my $af_h;
my $af_started = 0;

sub initSubChain {
	if ($ENV{'DEBUG'}) { print STDERR "$0 initSubChain enter\n"; }
	$af_h = start \@af_cmd, \$af_in, \$af_out, \$af_err;
	$af_started = 1;
	if ($ENV{'DEBUG'}) { print STDERR "$0 initSubChain exit\n"; }
}

sub callSubChain {
	if (!$af_started) {
		initSubChain();
	}
	my ($input) = @_;
	if ($ENV{'DEBUG'}) { print STDERR "$0 callSubChain input: $input\n"; }
	utf8::encode($input);
	$af_in .= $input;
	$af_in .= "\n\n<STREAMCMD:FLUSH>\n\n";
	pump $af_h until $af_out =~ /<STREAMCMD:FLUSH>/g;

	my $out = $af_out;
	$af_out = '';

	utf8::decode($out);
	$out =~ s@</s>@@g;
	$out =~ s/<STREAMCMD:FLUSH>//g;
	$out =~ s@^\s+@@g;
	$out =~ s@\s+$@@g;
	if ($ENV{'DEBUG'}) { print STDERR "$0 callSubChain output: $out\n"; }

	return $out;
}

binmode(STDIN);
binmode(STDOUT);

if (!check_protocol(*STDIN)) {
   die("Out of date protocol!\n");
}

while (my $w = read_window(*STDIN)) {
   my $out = '';
   foreach my $c (@{$w->{'cohorts'}}) {
      $out .= $c->{'wordform'}."\n";
      foreach my $r (@{$c->{'readings'}}) {
         $out .= "\t".$r->{'baseform'};
         foreach my $t (@{$r->{'tags'}}) {
            $out .= ' '.$t;
         }
         $out .= "\n";
      }
   }

   my $in = callSubChain($out);

	$out =~ s@^\s+@@g;
	$out =~ s@\s+$@@g;

   if ($in eq $out) { # No change, so just skip the rest.
      write_null_response(*STDOUT);
      next;
   }

   my @out = split /\n/, $out;

   my @in = split /\n/, $in;

   my $lout = @out;
   my $lin = @in;
   if ($lout != $lin) {
      print STDERR "Mismatch in number of lines!\n";
      write_null_response(*STDOUT);
      next;
   }

   my $cc = 0;
   for (my $i = 0 ; $i<$lin ; $i++) {
      if ($in[$i] !~ /\t/) { # Found a cohort line, start looking for readings
         $cc++;
         my $c = @{$w->{'cohorts'}}[$cc-1];

         $in[$i] =~ s/^\s+//g;
         $in[$i] =~ s/\s+$//g;
         if ($in[$i] ne $out[$i]) { # Wordform changed
            $c->{'wordform'} = $in[$i];
         }

         my $rc = 0;
         my $j;
         for ($j = $i+1 ; $j<$lin ; $j++) {
            if ($in[$j] !~ /\t/) { # Found a cohort line, so stop looking for readings
               last;
            }
            $rc++;
            if ($in[$j] eq $out[$j]) { # Skip if the reading has no changes
               next;
            }
            my $r = @{$c->{'readings'}}[$rc-1];
            $r->{'flags'} |= (1 << 0);

            $in[$j] =~ s/^\s+//g;
            $in[$j] =~ s/\s+$//g;
            my @tags = split /\s+/, $in[$j];
            $r->{'baseform'} = shift(@tags);
            @{$r->{'tags'}} = @tags;
         }
         $i = $j-1;
      }
   }

   write_window(*STDOUT, $w);
}
