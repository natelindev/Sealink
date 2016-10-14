#include "common.h"
#include <openssl/md5.h>

static MYSQL* sqlconn = NULL;

MYSQL* db_EstablishConnection(char* target, unsigned int port, char* user, char* password, char* db) {
    MYSQL* result;

    //if( sqlconn ) 	return sqlconn;
    result = mysql_init(NULL);
    if(!result) {
        LogEvent(LOG_ERROR, "mysql_init() failed : %s", mysql_error(result));
        return NULL;
    }
    if(mysql_real_connect(result, target, user, password, db, port, NULL, 0) == NULL) {
        LogEvent(LOG_ERROR, "mysql_real_connect() failed : %s", mysql_error(result));
        mysql_close(result);
        return NULL;
    }
    //sqlconn = result;
    return result;
}

/*
create table user ( id INT NOT NULL auto_increment, username varchar(25) not null, passwd char(32) not null, primary key (`id`) );
*/

static int db_QuerySanitize(char* str,int len) {
    // check
    int i;
    for(i=0;i<len;i++) {
        if(!isalnum(str[i]))    return 0;
    }
    return 1;
}

int db_InsertUser(MYSQL* conn, char* name,char* passwd) {
    const char* insert_user = "insert into user(username,passwd) value ('%s','%s');";
    // sanity check
    if(strlen(name) > 25)   return -1;
    if(!db_QuerySanitize(name,strlen(name))) {
        LogEvent(LOG_WARNING, "Invalid name : %s", name);
        return -1;
    }
    uint8_t buf[40], buf2[40];
    md5(passwd, strlen(passwd), buf, 40);
    md5(buf, 32, buf2, 40);     // md5(md5(passwd));
    buf2[32] = 0;
    char* query;
    asprintf(&query, insert_user, name, buf2);
    LogEvent(LOG_INFO, "Query : %s", query);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        result = -1;
    }
    return result;
}

int db_LoginCheck(MYSQL* conn, uint32_t uid, char* passwd) {
    int result = -1;
    const char* query_user = "select passwd from user where id=%d;";
    uint8_t buf[40], buf2[40];

    if(strlen(passwd) > 32)     return -1;
    md5(passwd, strlen(passwd), buf, 40);
    md5(buf, 32, buf2, 40);     // md5(md5(passwd));

    char* query;
    asprintf(&query, query_user, uid);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    MYSQL_RES* query_result = mysql_use_result(conn);
    if(!query_result) {
        LogEvent(LOG_ERROR, "mysql_use_result() failed : %s", mysql_error(conn));
        return -1;
    }

    result = -1;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(query_result))) {
        uint32_t fcount = mysql_field_count(conn);
        LogEvent(LOG_INFO,"FIELD : %d %s", fcount, row[0]);
        if(strcmp(buf2, row[0]) == 0) {
            LogEvent(LOG_INFO, "Login Success : %s = md5(md5(%s))", row[0], passwd);
            result = 0;
        }
    }
    mysql_free_result(query_result);
    return result;
}

char* db_GetUsernameByUid(MYSQL* conn, uint32_t uid) {
    const char* query_uname = "select username from user where id=%d";

    int result = -1;
    char* query;
    asprintf(&query, query_uname, uid);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    MYSQL_RES* query_result = mysql_use_result(conn);
    if(!query_result) {
        LogEvent(LOG_ERROR, "mysql_use_result() failed : %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row;
    char* returned = NULL;
    if((row = mysql_fetch_row(query_result))) {
        returned = strdup(row[0]);
    }
    mysql_free_result(query_result);
    return returned;
}

int db_UpdateUsernameByUid(MYSQL* conn, uint32_t uid, char* username) {
    const char* update_uname = "update user set username='%s' where id=%d";

    int result = -1;
    char* query;
    if(strlen(username) > 25)   return -1;
    if(db_QuerySanitize(username, strlen(username)) == 0) {
        return -1;
    }
    asprintf(&query, update_uname, username, uid);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    } 
    return 0;
}

/*
 * create table groups ( gid INT(11) NOT NULL, uid INT(11) NOT NULL, PRIMARY KEY (gid, uid) );
 * alter table groups add constraint FK_c1 foreign key (uid) references user (id);
 */

int db_GetGroup(MYSQL* conn, uint32_t gid, uint32_t* usercount, uint32_t** users) {
    int result = -1;
    const char* query_group = "select uid from groups where gid = %d;";

    char* query;
    asprintf(&query, query_group, gid);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    MYSQL_RES* qresult = mysql_store_result(conn);
    *usercount = mysql_num_rows(qresult);
    *users = malloc(sizeof(uint32_t)*(*usercount));
    uint32_t i;
    MYSQL_ROW r;
    for(i=0;i<(*usercount);i++) {
        r = mysql_fetch_row(qresult);
        (*users)[i] = atoi(r[0]);
    }

    mysql_free_result(qresult);
    return 0;
}

int db_CreateGroup(MYSQL* conn, uint32_t gid, uint32_t usercount, uint32_t* users) {
    const char* insert_group = "insert into groups(gid,uid) value (%d,%d);";
    uint32_t i;
    char* query;
    for(i=0;i<usercount;i++) {
        asprintf(&query, insert_group, gid, users[i]);
        int result = mysql_query(conn, query);
        if(result != 0) {
            LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        }
        free(query);
    }

    return 0;
}

int db_GetUserGroups(MYSQL* conn, uint32_t uid, uint32_t* groupcnt, uint32_t** groups) {
    const char* query_usergrp = "select gid from groups where uid = %d;";

    char* query;
    asprintf(&query, query_usergrp, uid);
    int result = mysql_query(conn, query);
    free(query);

    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* qresult = mysql_store_result(conn);
    *groupcnt = mysql_num_rows(qresult);
    *groups = malloc(sizeof(uint32_t)*(*groupcnt));

    uint32_t i;
    MYSQL_ROW row;
    for(i=0;i<(*groupcnt);i++) {
        row = mysql_fetch_row(qresult);
        (*groups)[i] = atoi(row[0]); 
    }
    mysql_free_result(qresult);

    return 0;
}

/*
 * create table friends ( uid1 int(11) not null, uid2 int(11) not null );
 * primary key : (uid1, uid2)
 */

int db_AddFriends(MYSQL* conn, uint32_t uid1, uint32_t uid2) {
    const char* insert_friend = "insert into friends(uid1, uid2) value (%d,%d);";
    
    char* query;
    asprintf(&query, insert_friend, uid1, uid2);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }

    asprintf(&query, insert_friend, uid2, uid1);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    return 0;
}

int db_QueryFriends(MYSQL* conn, uint32_t uid, uint32_t* friend_count, uint32_t** friends) {
    const char* query_friend = "select uid2 from friends where uid1 = %d;";

    char* query;
    asprintf(&query, query_friend, uid);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    MYSQL_RES* qresult = mysql_store_result(conn);
    if(!qresult) {
        LogEvent(LOG_ERROR, "mysql_use_result() failed : %s", mysql_error(conn));
        return -1;
    }
    *friend_count = mysql_num_rows(qresult);
    LogEvent(LOG_DEBUG, "Affected rows : %d", *friend_count);

    (*friends) = (uint32_t*)malloc(sizeof(uint32_t)* (*friend_count));
    uint32_t i;
    MYSQL_ROW row;
    for(i=0;i<(*friend_count);i++) {
        row = mysql_fetch_row(qresult);
        (*friends)[i] = atoi(row[0]);
    }
    mysql_free_result(qresult);
    return 0;
}

int db_DeleteFriend(MYSQL* conn, uint32_t uid1, uint32_t uid2) {
    const char* delete_friend = "delete from friends where uid1 = %d and uid2 = %d;";

    char* query;
    asprintf(&query, delete_friend, uid1, uid2);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }

    asprintf(&query, delete_friend, uid2, uid1);
    result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    return 0;
}

int db_GetAvatar(MYSQL* conn, uint32_t uid, uint32_t* avatar_id) {
    const char* query_avatar = "select avatar_id from user where id = %d;";

    char* query;
    asprintf(&query, query_avatar, uid);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES* qresult = mysql_store_result(conn);
    if(qresult == NULL) {
        LogEvent(LOG_ERROR, "mysql_store_result() failed : %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(qresult);
    if(row) {
        *avatar_id = atoi(row[0]);
        result = 0;
    } else {
        result = -1;
    }

    mysql_free_result(qresult);
    return result;
}

int db_UpdateAvatar(MYSQL* conn, uint32_t uid, uint32_t avatar_id) {
    const char* update_avatar = "update user set avatar_id = %d where id = %d;";

    char* query;
    asprintf(&query, update_avatar, avatar_id, uid);
    int result = mysql_query(conn, query);
    free(query);
    if(result != 0) {
        LogEvent(LOG_ERROR, "mysql_query() failed : %s", mysql_error(conn));
        return -1;
    }
    return 0;
}