create database chatsystem;

create table user (
    userid int(11) not null,
    nickname varchar(20) not null,
    school varchar(20) not null,
    telnum char(11) not null,
    passwd varchar(100) not null,
    m_time timestamp not null default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
    primary key (userid),
    unique key telnum (telnum)
)ENGINE=InnoDB default charset=utf8;

create table friendinfo (
    userid int(11) not null,
    friend int(11) not null,
    KEY userid (userid),
    KEY friend (friend),
    foreign key(userid) references  user(userid),
    foreign key(friend) references  user(userid)
)ENGINE=InnoDB default charset=utf8;
