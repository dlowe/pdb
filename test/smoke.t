#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;

BEGIN { plan tests => 6 }

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
