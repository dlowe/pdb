#!/usr/bin/perl

package MySQLTest;

use strict;
use warnings;

our $basedir = '/opt/local/';
our $socket_file = '/socket';
our $pid_file = '/pid';

our @servers = (
    { 'name'         => 'master',
      'dir'          => '/tmp/test_master',
      'port'         => 1234,
      'init'         => 'test/mysql_master.sql',
      'partition_id' => 'master' },
    { 'name'         => 'partition_1',
      'dir'          => '/tmp/test_partition_1',
      'port'         => 1235,
      'init'         => 'test/mysql_partition_1.sql',
      'partition_id' => 1 },
    { 'name'         => 'partition_2',
      'dir'          => '/tmp/test_partition_2',
      'port'         => 1236,
      'init'         => 'test/mysql_partition_2.sql',
      'partition_id' => 2 },
);

our $database_configuration = qq#
db_type = mysql
#;

foreach my $server (@servers) {
    $database_configuration .= qq#
delegate $server->{'name'}
{
    partition_id = $server->{'partition_id'}
    hostname = 127.0.0.1
    port = $server->{'port'}
    name = $server->{'name'}
}

partitioned_table widget
{
    key = widget_id
}

map_table widget_map
{
    key = widget_id
    partition_id = partition_id
}
#;
}

1;
