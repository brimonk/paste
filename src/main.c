/*
 * Brian Chrzanowski
 * 2021-03-07 02:05:23
 *
 * POST    /new			returns the key for the object
 * GET     /id			returns the file at the id
 * PUT     /id			updates the data at the key with the PUT body
 * DELETE  /id			removes the data at the given key
 */

#define COMMON_IMPLEMENTATION
#include "common.h"

#include "key.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <fcgi_stdio.h>

#include <magic.h>

static magic_t MAGIC_COOKIE;

char *ENVVARS[] = {
    "DOCUMENT_ROOT",
    "GATEWAY_INTERFACE",
    "HTTPS",
    "HTTP_COOKIE",
    "HTTP_HOST",
    "HTTP_REFERER",
    "HTTP_USER_AGENT",
    "PATH",
	"CONTENT_LENGTH",
	"CONTENT_TYPE",
    "QUERY_STRING",
    "REMOTE_ADDR",
    "REMOTE_HOST",
    "REMOTE_PORT",
    "REMOTE_USER",
    "REQUEST_METHOD",
    "REQUEST_URI",
    "SCRIPT_FILENAME",
    "SCRIPT_NAME",
    "SERVER_ADMIN",
    "SERVER_NAME",
    "SERVER_PORT",
    "SERVER_SOFTWARE"
};

enum {
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_UNDEFINED,
	METHOD_TOTAL
};

enum {
	ERROR_NONE,
	ERROR_NULL_URI,
	ERROR_ZERO_LENGTH_URI,
	ERROR_NO_KEY_GIVEN,
	ERROR_ILLEGAL_KEY,
	ERROR_KEY_NOT_FOUND,
	ERROR_POST_BADPATH,
	ERROR_TOTAL
};

char *ERRSTRS[] = {
	"Cannot operate correctly, no error here.\n", // ERROR_NONE
	"Cannot operate with a NULL URI.\n", // ERROR_NULL_URI
	"Cannot operate with a zero length URI.\n", // ERROR_ZERO_LENGTH_URI
	"Cannot operate without a key.\n", // ERROR_NO_KEY_GIVEN
	"Cannot operate with an illegal key.\n", // ERROR_ILLEGAL_KEY
	"Cannot operate with a nonexistent key.\n", // ERROR_KEY_NOT_FOUND
	"Cannot operate with this path. Use /upload instead.\n", // ERROR_POST_BADPATH
};

// Request: a structure to hold everything needed for the web request
struct Request {
	int Method;

	char *Uri;

	s64 ContentLen;
	char *ContentType;

	char *Body;
	// size_t BodyLen;
	// size_t BodyCap;

	char *Host;
};

// Request Methods
// ParseRequest: returns a request structure
int ParseRequest(struct Request *req);
// ParseMethod: returns the enum for the request method
int ParseMethod(char *request);
// GetRequestBody: gets the request body, if there is one
void *GetRequestBody(size_t bytes);

// Application Logic Functions
// PerformProcessing: does the processing necessary, sets output buffers too
int PerformProcessing(struct Request *req);
// PerformGet: performs the GET action
int PerformGet(struct Request *req);
// PerformPost: creates a new file and sends the key to the user
int PerformPost(struct Request *req);
// PerformPut: updates the file at the key, and returns the key
int PerformPut(struct Request *req);
// PerformDelete: removes the item at the given key
int PerformDelete(struct Request *req);

// SendError: sends an error response in plain text
int SendError(int errcode);

// GetKey: returns the key from the uri
char *GetKey(char *uri);
// CheckForKeyErrors: checks for common errors with the input uri
int CheckForKeyErrors(char *uri);

// Helper Functions
// Bootstrap: bootstraps the program
void Bootstrap(void);
// CleanRequest: cleans up the request object
void CleanRequest(struct Request *req);

int main(int argc, char **argv)
{
	struct Request req;

	memset(&req, 0, sizeof req);

	Bootstrap();

	while (FCGI_Accept() >= 0) {
		CleanRequest(&req);
		ParseRequest(&req);
		PerformProcessing(&req);
	}

	return 0;
}

// SendError: sends an error response in plain text
int SendError(int errcode)
{
	printf("Content-type: text/plain\r\n");
	printf("\r\n");
	printf("%s", ERRSTRS[errcode]);

	return 0;
}

// PerformProcessing: does the processing necessary, sets output buffers too
int PerformProcessing(struct Request *req)
{
	int rc;

	if (req == NULL) {
		return -1;
	}

	rc = 0;

	switch (req->Method) {
	case METHOD_GET:
		rc = PerformGet(req);
		break;

	case METHOD_POST:
		rc = PerformPost(req);
		break;

	case METHOD_PUT:
		rc = PerformPut(req);
		break;

	case METHOD_DELETE:
		rc = PerformDelete(req);
		break;

	default:
	case METHOD_UNDEFINED:
		rc = -1;
		break;
	}

	return rc;
}

// PerformGet: retrieves the content for the key, and stores in the response
int PerformGet(struct Request *req)
{
	int rc;
	char *key;
	void *data;
	size_t len;

	rc = 0;

	rc = CheckForKeyErrors(req->Uri);
	if (rc != ERROR_NONE) {
		SendError(rc);
		return -1;
	}

	key = GetKey(req->Uri);
	data = KeyGet(key, &len);

	printf("Content-Length: %ld\r\n", len);
	printf("Content-Type: %s\r\n", magic_buffer(MAGIC_COOKIE, data, len));
	printf("\r\n");

	FCGI_fwrite(data, 1, len, stdout);

	return rc;
}

// PerformPost: creates a new file and sets the key in the response
int PerformPost(struct Request *req)
{
	char key[BUFSMALL];

	if (!streq(req->Uri, "/upload")) {
		SendError(ERROR_POST_BADPATH);
		return -1;
	}

	memset(key, 0, sizeof key);

	KeyMake(key, KEYLEN);

	KeyWrite(key, req->Body, req->ContentLen);

	printf("Content-Type: text/plain\r\n");
	printf("\r\n");
	printf("https://%s/%s\n", req->Host, key);

	return 0;
}

// PerformPut: updates the file at the key, and returns the key
int PerformPut(struct Request *req)
{
	char *key;
	int rc;

	rc = CheckForKeyErrors(req->Uri);
	if (rc != ERROR_NONE) {
		SendError(rc);
		return -1;
	}

	key = GetKey(req->Uri);

	KeyDelete(key);
	KeyWrite(key, req->Body, req->ContentLen);

	printf("Content-Type: text/plain\r\n");
	printf("\r\n");
	printf("https://%s/%s\n", req->Host, key);

	return 0;
}

// PerformDelete: removes the item at the given key
int PerformDelete(struct Request *req)
{
	char *key;
	int rc;

	rc = CheckForKeyErrors(req->Uri);
	if (rc != ERROR_NONE) {
		SendError(rc);
		return -1;
	}

	key = GetKey(req->Uri);
	rc = KeyDelete(key);

	printf("Content-Type: text/plain\r\n");
	printf("\r\n");
	printf("success\n");

	return 0;
}

// GetKey: returns the key from the uri
char *GetKey(char *uri)
{
	return uri + 1;
}

// CheckForKeyErrors: checks for common errors with the input uri
int CheckForKeyErrors(char *uri)
{
	char *key;
	size_t keylen;

	if (uri == NULL) {
		return ERROR_NULL_URI;
	}

	if (strlen(uri) == 0) {
		return ERROR_ZERO_LENGTH_URI;
	}

	if (strlen(uri) == 1 && uri[0] == '/') {
		return ERROR_NO_KEY_GIVEN;
	}

	key = uri + 1;
	keylen = strlen(key);

	if (!KeyIsLegal(key, keylen)) {
		return ERROR_ILLEGAL_KEY;
	}

	if (!KeyDoesExist(key)) {
		return ERROR_KEY_NOT_FOUND;
	}

	return ERROR_NONE;
}

// ParseRequest: returns a request structure
int ParseRequest(struct Request *req)
{
	char *s;

	memset(req, 0, sizeof(*req));

	s = getenv("REQUEST_METHOD");
	if (s) {
		req->Method = ParseMethod(s);
	}

	s = getenv("REQUEST_URI");
	if (s) {
		req->Uri = strdup(s);
	}

	s = getenv("CONTENT_TYPE");
	if (s) {
		req->ContentType = strdup(s);
	}

	s = getenv("CONTENT_LENGTH");
	if (s) {
		req->ContentLen = atoll(s);
	}

	s = getenv("HTTP_HOST");
	if (s) {
		req->Host = strdup(s);
	}

	req->Body = GetRequestBody(req->ContentLen);

	return 0;
}

// ParseMethod: returns the enum for the request method
int ParseMethod(char *request)
{
	char *map[] = { // aligns with the enum values
		"GET", "POST", "PUT", "DELETE"
	};
	int i;

	if (request == NULL) {
		return METHOD_UNDEFINED;
	}

	for (i = 0; i < ARRSIZE(map); i++) {
		if (streq(map[i], request)) {
			return i;
		}
	}

	return METHOD_UNDEFINED;
}

// GetRequestBody: gets the request body, if there is one
void *GetRequestBody(size_t bytes)
{
	void *p;

	p = calloc(1, bytes);
	FCGI_fread(p, 1, bytes, FCGI_stdin);

	return p;
}

// Bootstrap: bootstraps the program
void Bootstrap(void)
{
	int rc;

	// create a directory to store all of the junk
	rc = mkdir(DATADIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (rc < 0) {
		perror(strerror(rc));
	}

	// seed the rng machine if it hasn't been
	pcg_seed(&localrand, time(NULL) ^ (long)printf, (unsigned long)Bootstrap);

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
}

// CleanRequest: cleans up the request object
void CleanRequest(struct Request *req)
{
	if (req) {
		free(req->Uri);
		free(req->Body);

		memset(req, 0, sizeof(*req));
	}
}

