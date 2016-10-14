#ifndef _COMMON_H
#error "Do NOT include this header solely."
#endif

/* TODO : Add database config */
#define DB_USERNAME     "linpop"
#define DB_PASSWORD     "linpop"
#define DB_HOST         "127.0.0.1"
#define DB_PORT         3306
#define DB_MAINDB       "linpop"

MYSQL* db_EstablishConnection(char* target, unsigned int port, char* user, char* password, char* db);

int db_InsertUser(MYSQL* conn, char* name,char* passwd);
int db_LoginCheck(MYSQL* conn, uint32_t uid, char* passwd);

char* db_GetUsernameByUid(MYSQL* conn, uint32_t uid);
int db_UpdateUsernameByUid(MYSQL* conn, uint32_t uid, char* username);

int db_AddFriends(MYSQL* conn, uint32_t uid1, uint32_t uid2);
int db_QueryFriends(MYSQL* conn, uint32_t uid, uint32_t* friend_count, uint32_t** friends);
int db_DeleteFriend(MYSQL* conn, uint32_t uid1, uint32_t uid2);

int db_GetGroup(MYSQL* conn, uint32_t gid, uint32_t* usercount, uint32_t** users);
int db_CreateGroup(MYSQL* conn, uint32_t gid, uint32_t usercount, uint32_t* users);
int db_GetUserGroups(MYSQL* conn, uint32_t uid, uint32_t* groupcnt, uint32_t** groups);

int db_GetAvatar(MYSQL* conn, uint32_t uid, uint32_t* avatar_id);
int db_UpdateAvatar(MYSQL* conn, uint32_t uid, uint32_t avatar_id);