#!/usr/bin/perl

use strict;
use warnings;

package PDBTest;

our $pdb = "./pdb";

sub exists {
    return -x $pdb;
}

sub startup {
    my $output = `$pdb 2>&1`;
    die $output if $output ne '';
    return pid();
}

sub pid {
    my $pid = `ps -ax | grep $pdb | grep -v grep | head -1 | awk '{print \$1}'`;
    chomp $pid;
    return $pid;
}

sub shutdown {
    my $pid = pid();
    my $output = `kill $pid`;
    die $output if $output ne '';
    select(undef, undef, undef, 0.250);
    $pid = pid();
    die "pdb didn't die: $pid" if $pid ne '';
}

1;
