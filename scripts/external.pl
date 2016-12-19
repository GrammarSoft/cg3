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

binmode(STDIN);
binmode(STDOUT);

if (!check_protocol(*STDIN)) {
   die("Out of date protocol!\n");
}

while (my $w = read_window(*STDIN)) {
   foreach my $c (@{$w->{'cohorts'}}) {
      #print $c->{'wordform'}, "\n";
      foreach my $r (@{$c->{'readings'}}) {
         #print "\t", $r->{'baseform'}, "\n";
         $r->{'flags'} |= (1 << 0);
         push @{$r->{'tags'}}, "æ発ø";
         foreach my $t (@{$r->{'tags'}}) {
            #print "\t\t", $t, "\n";
         }
      }
   }
   write_window(*STDOUT, $w);
}
