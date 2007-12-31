create database partition_2;

connect partition_2;

create table widget (
    widget_id INTEGER NOT NULL PRIMARY KEY,
    widget_information VARCHAR(256) NOT NULL
);

insert into widget values (3, 'widget three');
insert into widget values (4, 'widget four');

commit;
