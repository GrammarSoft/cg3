#!/usr/bin/perl
# -*- mode: cperl; indent-tabs-mode: nil; tab-width: 3; cperl-indent-level: 3; -*-
package CG3_External;
use strict;
use warnings;
use utf8;
use feature 'unicode_strings';

our $VERSION = 7226;
our @EXPORT_OK;

BEGIN {
   use Exporter ();
   our (@ISA, @EXPORT_OK);

   @ISA = qw(Exporter);

   @EXPORT_OK = qw(&check_protocol &read_window &write_window);
}

sub read_uint16_t {
   my ($fh,$msg) = @_;

   my $n = 0;
   if (sysread($fh, $n, 2) != 2) {
      die("Failed to read $msg uint16_t!\n");
   }
   $n = unpack('S', $n);
   if ($ENV{'DEBUG'}) { print STDERR "Read $msg uint16_t: $n\n"; }
   return $n;
}

sub write_uint16_t {
   my ($fh,$n,$msg) = @_;

   if ($ENV{'DEBUG'}) { print STDERR "Writing $msg uint16_t: $n\n"; }
   $n = pack('S', $n);
   if (syswrite($fh, $n, 2) != 2) {
      die("Failed to write $msg uint16_t!\n");
   }
}

sub read_uint32_t {
   my ($fh,$msg) = @_;

   my $n = 0;
   if (sysread($fh, $n, 4) != 4) {
      die("Failed to read $msg uint32_t!\n");
   }
   $n = unpack('L', $n);
   if ($ENV{'DEBUG'}) { print STDERR "Read $msg uint32_t: $n\n"; }
   return $n;
}

sub write_uint32_t {
   my ($fh,$n,$msg) = @_;

   if ($ENV{'DEBUG'}) { print STDERR "Writing $msg uint32_t: $n\n"; }
   $n = pack('L', $n);
   if (syswrite($fh, $n, 4) != 4) {
      die("Failed to write $msg uint32_t!\n");
   }
}

sub read_utf8_string {
   my ($fh,$msg) = @_;

   my $n = read_uint16_t($fh, $msg.' length');
   my $s = '';
   if (sysread($fh, $s, $n) != $n) {
      die("Failed to read $msg UTF-8 string!\n");
   }
   if ($ENV{'DEBUG'}) { print STDERR "Read $msg UTF-8 string: $s\n"; }
   return $s;
}

sub write_utf8_string {
   my ($fh,$s,$msg) = @_;

   my $n = length($s);
   write_uint16_t($fh, $n, $msg.' length');
   if ($ENV{'DEBUG'}) { print STDERR "Writing $msg UTF-8 string: $s\n"; }
   if (syswrite($fh, $s, $n) != $n) {
      die("Failed to write $msg UTF-8 string!\n");
   }
}

sub check_protocol {
   my ($fh) = @_;

   my $protocol = read_uint32_t($fh, 'protocol revision');
   if ($protocol == $VERSION) {
      return $VERSION;
   }
   
   return undef;
}

sub read_window {
   my ($fh) = @_;

   my $len = read_uint32_t($fh, 'window packet length');
   if ($len == 0) {
      return undef;
   }

   my %window = ();
   $window{'num'} = read_uint32_t($fh, 'window number');

   my @cohorts;
   my $clen = read_uint32_t($fh, 'num cohorts');
   for (my $c=0 ; $c<$clen ; $c++) {
      read_uint32_t($fh, 'cohort packet length');
      my %cohort = ();
      $cohort{'num'} = read_uint32_t($fh, 'cohort number');
      $cohort{'flags'} = read_uint32_t($fh, 'cohort flags');
      if ($cohort{'flags'} & (1 << 1)) {
         $cohort{'parent'} = read_uint32_t($fh, 'cohort parent');
      }
      $cohort{'wordform'} = read_utf8_string($fh, 'cohort wordform');
      
      my @readings;
      my $rlen = read_uint32_t($fh, 'num readings');
      for (my $r=0 ; $r<$rlen ; $r++) {
         read_uint32_t($fh, 'reading packet length');
         my %reading = ();
         $reading{'flags'} = read_uint32_t($fh, 'reading flags');

         if ($reading{'flags'} & (1 << 3)) {
            $reading{'baseform'} = read_utf8_string($fh, 'reading baseform');
         }
         
         my @tags;
         my $tlen = read_uint32_t($fh, 'num tags');
         for (my $t=0 ; $t<$tlen ; $t++) {
            my $tag = read_utf8_string($fh, 'tag');
            push @tags, $tag;
         }
         $reading{'tags'} = \@tags;

         push @readings, \%reading;
      }
      $cohort{'readings'} = \@readings;

      if ($cohort{'flags'} & (1 << 0)) {
         $cohort{'text'} = read_utf8_string($fh, 'cohort text');
      }

      push @cohorts, \%cohort;
   }
   $window{'cohorts'} = \@cohorts;
   
   return \%window;
}

sub write_window {
   my ($fh,$w) = @_;

   write_uint32_t($fh, 0, 'window packet length');
}

1;
