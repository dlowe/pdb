create database master;

connect master;

create table widget_map (
    widget_id INTEGER NOT NULL PRIMARY KEY,
    partition_id INTEGER NOT NULL
);

insert into widget_map values (1, 1);
insert into widget_map values (2, 1);
insert into widget_map values (3, 2);
insert into widget_map values (4, 2);

create table whatsit (
    whatsit_id INTEGER NOT NULL PRIMARY KEY,
    description VARCHAR(256) NOT NULL
);

insert into whatsit values (1, 'boot');

commit;
