/*
 * Brian Chrzanowski
 * 2021-03-07 02:05:23
 *
 * my pastebin, with a small, static, instructional webpage
 */

#define COMMON_IMPLEMENTATION
#include "common.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include "sqlite3.h"

#include <magic.h>

static magic_t MAGIC_COOKIE;

char KEYMAP[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
	'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
};

sqlite3 *db;

// init: initializes the program
void init(char *db_file_name, char *sql_file_name);

void cleanup(void);

// create_tables: execs create * statements on the database
int create_tables(sqlite3 *db, char *fname);

#define SQLITE_ERR(x) (fprintf(stderr, "Error: %s\n", sqlite3_errstr(rc)))

#define USAGE ("USAGE: %s <dbname>\n")

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1], "schema.sql");

	int i, rc;
	char *err;

	for (i = 0; i < 100; i++) {
#define TEST_SQL ("insert into pastes (data) values ((randomblob(32)));")
		rc = sqlite3_exec(db, TEST_SQL, NULL, NULL, &err);
	}

	cleanup();

	return 0;
}

// init: initializes the program
void init(char *db_file_name, char *sql_file_name)
{
	int rc;

	// seed the rng machine if it hasn't been
	pcg_seed(&localrand, time(NULL) ^ (long)printf, (unsigned long)init);

	// setup libmagic
	MAGIC_COOKIE = magic_open(MAGIC_MIME);
	if (MAGIC_COOKIE == NULL) {
		fprintf(stderr, "%s", magic_error(MAGIC_COOKIE));
		exit(1);
	}

	if (magic_load(MAGIC_COOKIE, NULL) != 0) {
		fprintf(stderr, "cannot load magic database - %s\n", magic_error(MAGIC_COOKIE));
		magic_close(MAGIC_COOKIE);
	}

	rc = sqlite3_open(db_file_name, &db);
	if (rc != SQLITE_OK) {
		SQLITE_ERR(rc);
		exit(1);
	}

	sqlite3_db_config(db,SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERR(rc);
		exit(1);
	}

	rc = sqlite3_load_extension(db, "./ext_uuid.so", "sqlite3_uuid_init", NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERR(rc);
		exit(1);
	}

	rc = create_tables(db, sql_file_name);
	if (rc < 0) {
		ERR("Critical error in creating sql tables!!\n");
		exit(1);
	}
}

// cleanup: cleans up for a shutdown (probably doesn't ever happen)
void cleanup(void)
{
	sqlite3_close(db);
}

// create_tables: bootstraps the database (and the rest of the app)
int create_tables(sqlite3 *db, char *fname)
{
	sqlite3_stmt *stmt;
	char *sql, *s;
	int rc;

	sql = sys_readfile(fname, NULL);
	if (sql == NULL) {
		return -1;
	}

	for (s = sql; *s;) {
		rc = sqlite3_prepare_v2(db, s, -1, &stmt, (const char **)&s);
		if (rc != SQLITE_OK) {
			ERR("Couldn't Prepare STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}

		rc = sqlite3_step(stmt);

		//  TODO (brian): goofy hack, ensure that this can really just run
		//  all of our bootstrapping things we'd ever need
		if (rc == SQLITE_MISUSE) {
			continue;
		}

		if (rc != SQLITE_DONE) {
			ERR("Couldn't Execute STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}

		rc = sqlite3_finalize(stmt);
		if (rc != SQLITE_OK) {
			ERR("Couldn't Finalize STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}
	}

	return 0;
}

