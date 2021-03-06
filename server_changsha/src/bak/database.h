

#ifndef __DATABASE_H_
#define __DATABASE_H_
////////////////////////////////////////////////////////////////////////////////
#include <inc/sqlnet.h>
////////////////////////////////////////////////////////////////////////////////
bool init_database();
void free_database();
bool open_database(sqlnet3 &db, const char *name);
void close_database(sqlnet3 &db);
bool copy_player_data();
////////////////////////////////////////////////////////////////////////////////
#endif //__DATABASE_H_
