#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;

BEGIN { plan tests => 5 }

require('test/PDBTest.pm');

my $pdb = "./pdb";
ok(PDBTest::exists(), 1, "can't find pdb") or die;

my $pid;
eval {
    $pid = PDBTest::startup();
};
ok($@, '', "pdb failed to boot: $@") or die;

ok($pid, qr/^\d+$/, "can't figure out pdb process id");

eval {
    socket(SH, PF_INET, SOCK_STREAM, getprotobyname('tcp')) or die;
    connect(SH, sockaddr_in(5032, inet_aton("localhost"))) or die;
    close(SH) or die;
};
ok($@, '', "pdb not answering: $@");

eval {
    PDBTest::shutdown();
};
ok($@, '', "shutdown failed: $@");
