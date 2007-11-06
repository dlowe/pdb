#!/usr/bin/perl

use strict;
use warnings;

package PDBTest;

our $pid = "./pdb";

sub startup {
    my $output = `$pdb 2>&1`;
    die $output if $output ne '';
    return pid();
}

sub pid {
    my $pid = `ps -ax | grep $pdb | grep -v grep | awk '{print \$1}'`;
    chomp $pid;
    return $pid;
}

sub shutdown {
    my $pid = pid();
    my $output = `kill $pid`;
    die $output if $output ne '';
    select(undef, undef, undef, 0.250);
    $pid = pid();
    die if $pid ne '';
}

my $pdb = "./pdb";
ok(-x $pdb, 1, "can't find pdb") or die;

my $output = `$pdb 2>&1`;
ok($output, '', "pdb failed to boot: $output") or die;

my $pid = `ps -ax | grep $pdb | grep -v grep | awk '{print \$1}'`;
chomp $pid;
ok($pid, qr/^\d+$/, "can't figure out pdb process id");

socket(SH, PF_INET, SOCK_STREAM, getprotobyname('tcp'));
connect(SH, sockaddr_in(5032, inet_aton("localhost")));
ok(<SH>, "poot!\n", 'pdb not answering');

$output = `kill $pid`;
ok($output, '', "unable to kill $pid: $output");

# admittedly hacky
select(undef, undef, undef, 0.250);

$pid = `ps -ax | grep $pdb | grep -v grep | awk '{print \$1}'`;
chomp $pid;
ok($pid, '', "pdb didn't die: $pdb");
