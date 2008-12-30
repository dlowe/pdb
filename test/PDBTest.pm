#!/usr/bin/perl

use strict;
use warnings;

use File::Temp ();

package PDBTest;

my $pdb = "./pdb";

sub exists () {
    return -x $pdb;
}

sub startup_with_args ($) {
    my $args = shift;
    my $output = `$pdb $args 2>&1`;
    die $output if $output ne '';
    return pid();
}

sub startup_with_inline_configuration ($) {
    my $cfg = shift;
    my $cfile = new File::Temp();
    print $cfile $cfg;
    startup_with_args('-c ' . $cfile->filename());
}

sub startup () {
    startup_with_args("-c test/pdb.cfg");
}

sub pid () {
    my $pid = `ps -ax | grep $pdb | grep -v grep | head -1 | awk '{print \$1}'`;
    chomp $pid;
    return $pid;
}

sub shutdown () {
    my $pid = pid();
    if ($pid) {
        my $output = `kill $pid`;
        die $output if $output ne '';
        select(undef, undef, undef, 0.50);
        my $newpid = pid();
        die "pdb didn't die: $newpid (killed $pid)" if $newpid ne '';
    }
}

1;
