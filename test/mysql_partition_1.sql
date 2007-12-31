create database partition_1;

connect partition_1;

create table widget (
    widget_id INTEGER NOT NULL PRIMARY KEY,
    widget_information VARCHAR(256) NOT NULL
);

insert into widget values (1, 'widget one');
insert into widget values (2, 'widget two');

commit;
