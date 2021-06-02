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

#define PORT (5000)

static magic_t MAGIC_COOKIE;

static sqlite3 *db;

// init: initializes the program
void init(char *db_file_name, char *sql_file_name);

void cleanup(void);

// create_tables: execs create * statements on the database
int create_tables(sqlite3 *db, char *fname);

// is_uuid: returns true if the input string is a uuid
int is_uuid(char *id);

// request_handler: the http request handler
void request_handler(struct http_request_s *req);

// SERVER FUNCTIONS
// send_paste: sends the given paste to the requester
int send_paste(struct http_request_s *req, struct http_response_s *res, char *id);
// add_paste: adds a paste into the database
int add_paste(char **id, void *blob, size_t len);
// get_paste: loads the entire blob into memory
int get_paste(char *id, void **blob, size_t *len);
// send_root: sends the root webpage (has instructions)
int send_root(struct http_request_s *req, struct http_response_s *res);
// send_error_internal: sends the root webpage (has instructions)
int send_error_internal(struct http_request_s *req, struct http_response_s *res);

#define SQLITE_ERRMSG(x) (fprintf(stderr, "Error: %s\n", sqlite3_errstr(rc)))

#define USAGE ("USAGE: %s <dbname>\n")

void test(void)
{
	int i, rc;
	char *err;

	for (i = 0; i < 100; i++) {
#define TEST_SQL ("insert into pastes (data) values ((randomblob(32)));")
		rc = sqlite3_exec(db, TEST_SQL, NULL, NULL, &err);
	}

	printf("%d\n", is_uuid("56ac4f6d-084f-4a45-8353-bbf59d872a68"));
}

int main(int argc, char **argv)
{
	struct http_server_s *server;
	int rc;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1], "schema.sql");

	server = http_server_init(PORT, request_handler);

	printf("listening on http://localhost:%d\n", PORT);

	// test();

	rc = http_server_listen(server);

	cleanup();

	return 0;
}

// request_handler: the http request handler
void request_handler(struct http_request_s *req)
{
	struct http_response_s *res;
	struct http_string_s m, t;
	struct http_string_s body;
	char *method, *target;
	char *id;
	int rc;
	char tbuf[BUFSMALL];

	res = http_response_init();

	m = http_request_method(req);
	t = http_request_target(req);
	body = http_request_body(req);

	method = strndup(m.buf, m.len);
	target = strndup(t.buf, t.len);

	if (streq(method, "GET")) {
		if (is_uuid(target + 1)) { // getting a paste
			rc = send_paste(req, res, target + 1);
			if (rc < 0) {
				send_error_internal(req, res);
			}
		} else {
			rc = send_root(req, res);
			if (rc < 0) {
				send_error_internal(req, res);
			}
		}
	} else if (streq(method, "POST")) {
		rc = add_paste(&id, (void *)body.buf, body.len);
		if (rc < 0) {
			send_error_internal(req, res);
		}

		http_response_status(res, 200);
		http_response_header(res, "Content-Type", "plain/text");

		snprintf(tbuf, sizeof tbuf, "http://localhost:%d/%s\n", PORT, id);

		http_response_body(res, tbuf, strlen(tbuf));

		http_respond(req, res);

		rc = send_paste(req, res, id);
		if (rc < 0) {
			send_error_internal(req, res);
		}

		free(id);
	} else {
		send_error_internal(req, res);
	}


	// just a small test

	free(target);
	free(method);
}

// add_paste: adds a paste into the database
int add_paste(char **id, void *blob, size_t len)
{
	sqlite3_stmt *stmt;
	char *err;
	const unsigned char *tid;
	size_t tlen;
	long rowid;
	int rc;

	// NOTE (Brian) to perform this semi-effectively, we insert the blob,
	// then we get the 'rowid' of the thing we _just_ inserted, and use that
	// to get the uuid that the database generated for it.

#define ADD_INSERT_SQL ("insert into pastes (data) values (?);")
#define ADD_SELECT_SQL ("select id from pastes where rowid = ?;")

	// we do the insert first
	rc = sqlite3_prepare_v2(db, ADD_INSERT_SQL, -1, &stmt, (const char **)&err);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_bind_blob(stmt, 1, blob, len, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_step(stmt);

	rowid = sqlite3_last_insert_rowid(db);

	sqlite3_finalize(stmt);

	// then we do the select
	rc = sqlite3_prepare_v2(db, ADD_SELECT_SQL, -1, &stmt, (const char **)&err);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_bind_int64(stmt, 1, rowid);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_step(stmt);

	sqlite3_column_text(stmt, 0);
	tlen = sqlite3_column_bytes(stmt, 0);
	*id = calloc(tlen, 1);
	tid = sqlite3_column_text(stmt, 0);

	memcpy(*id, tid, tlen);

	sqlite3_finalize(stmt);

	return 0;
}

// send_root: sends the root webpage (has instructions)
int send_root(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// send_error_internal: sends the root webpage (has instructions)
int send_error_internal(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// send_paste: sends the given paste to the requester
int send_paste(struct http_request_s *req, struct http_response_s *res, char *id)
{
	void *blob;
	size_t len;
	int rc;

	rc = get_paste(id, &blob, &len);
	if (rc < 0) {
		return rc;
	}

	http_response_status(res, 200);
	http_response_header(res, "Content-Type", "plain/text");
	http_response_body(res, blob, len);

	http_respond(req, res);

	free(blob);

	return 0;
}

// get_paste: loads the entire blob into memory
int get_paste(char *id, void **blob, size_t *len)
{
	int rc;
	sqlite3_stmt *stmt;
	const void *tblob;
	char *err;

	if (id == NULL) {
		return -1;
	}

#define GET_SQL ("select data from pastes where id = '?';")

	rc = sqlite3_prepare_v2(db, GET_SQL, -1, &stmt, (const char **)&err);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_bind_text(stmt, 1, id, strlen(id), NULL);

	rc = sqlite3_step(stmt);

	// TODO (brian) reasonably check for errors

	*len = sqlite3_column_bytes(stmt, 0);
	*blob = calloc(*len, 1);
	tblob = sqlite3_column_blob(stmt, 0);
	memcpy(*blob, tblob, *len);

	sqlite3_finalize(stmt);

	return 0;
}

// is_uuid: returns true if the input string is a uuid
int is_uuid(char *id)
{
	int i;

	if (id == NULL) {
		return 0;
	}

	if (strlen(id) < 36) {
		return 0;
	}

	for (i = 0; i < 36; i++) { // ensure our chars are right
		switch (id[i]) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9': case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case '-':
				break;
		default:
			return 0;
		}
	}

	// index 13 should always be '4'
	return id[14] == '4';
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
		SQLITE_ERRMSG(rc);
		exit(1);
	}

	sqlite3_db_config(db,SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		exit(1);
	}

	rc = sqlite3_load_extension(db, "./ext_uuid.so", "sqlite3_uuid_init", NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
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

