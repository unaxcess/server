alter table folder_message_item add threadid int not null default -1 after message_text;
alter table folder_message_item add archived int not null default 1 after threadid;
alter table folder_message_item add index archived_index(archived);



create table folder_message_save (
	userid int not null,
	messageid int not null
);
