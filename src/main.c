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
	ERROR_NOKEY,
	ERROR_ILLEGALKEY,
	ERROR_KEYNOTFOUND,
	ERROR_TOTAL
};

// Request: a structure to hold everything needed for the web request
struct Request {
	int Method;

	char *Uri;

	s64 ContentLen;
	char *ContentType;

	char *Body;
	size_t BodyLen;
	size_t BodyCap;
};

struct Response {
	void *content;
	int errcode;
};

// printallenv : prints all of the env vars
void printallenv(void)
{
	char *format;
	char *s;
	int i;

	format = "%s : %s\n";

	for (i = 0; i < ARRSIZE(ENVVARS); i++) {
		s = getenv(ENVVARS[i]);

		if (s) {
			printf(format, ENVVARS[i], s);
		} else {
			printf(format, ENVVARS[i], "(NULL)");
		}
	}
}

// Request Methods
// ParseRequest: returns a request structure
int ParseRequest(struct Request *req);
// ParseMethod: returns the enum for the request method
int ParseMethod(char *request);
// GetRequestBody: gets the request body, if there is one
void GetRequestBody(char **s, size_t *s_len, size_t *s_cap);

// Application Logic Functions
// PerformProcessing: does the processing necessary, sets output buffers too
int PerformProcessing(struct Response *res, struct Request *req);
// PerformGet: performs the GET action
int PerformGet(struct Response *res, struct Request *req);
// PerformPost: creates a new file and sets the key in the response
int PerformPost(struct Response *res, struct Request *req);
// PerformPut: updates the file at the key, and returns the key
int PerformPut(struct Response *res, struct Request *req);
// PerformDelete: removes the item at the given key
int PerformDelete(struct Response *res, struct Request *req);

// Response Methods
// SendResponse: sends the response
int SendResponse(struct Response *res);

// Helper Functions
// Bootstrap: bootstraps the program
void Bootstrap(void);
// CleanRequest: cleans up the request object
void CleanRequest(struct Request *req);
// CleanResponse: cleans up the response object
void CleanResponse(struct Response *res);

int main(int argc, char **argv)
{
	struct Request req;
	struct Response res;
	int rc;

	memset(&req, 0, sizeof req);
	memset(&res, 0, sizeof res);

	Bootstrap();

	while (FCGI_Accept() >= 0) {
		CleanRequest(&req);
		CleanResponse(&res);

		rc = ParseRequest(&req);
		if (rc < 0) {
			printf("Error Parsing Request!");
		}

		rc = PerformProcessing(&res, &req);
		if (rc < 0) {
			printf("Error Processing!");
		}

		rc = SendResponse(&res);
		if (rc < 0) {
			printf("Error Responding!");
		}
	}

	return 0;
}

// SendError: sends an error response in plain text
int SendError(int errcode);

// SendResponse: sends the response
int SendResponse(struct Response *res)
{
	if (res->errcode == ERROR_NONE) {
		if (res->content) // NOTE (Brian) temporary
			printf("%s", res->content);
	} else {
		SendError(res->errcode);
	}

	return 0;
}

// SendError: sends an error response in plain text
int SendError(int errcode)
{
	printf("Content-type: text/plain\r\n");
	printf("\r\n");

	return 0;
}

// PerformProcessing: does the processing necessary, sets output buffers too
int PerformProcessing(struct Response *res, struct Request *req)
{
	int rc;

	if (req == NULL || res == NULL) {
		return -1;
	}

	rc = 0;

	switch (req->Method) {
	case METHOD_GET:
		rc = PerformGet(res, req);
		break;

	case METHOD_POST:
		rc = PerformPost(res, req);
		break;

	case METHOD_PUT:
		rc = PerformPut(res, req);
		break;

	case METHOD_DELETE:
		rc = PerformDelete(res, req);
		break;

	default:
	case METHOD_UNDEFINED:
		rc = -1;
		break;
	}

	return rc;
}

// PerformGet: retrieves the content for the key, and stores in the response
int PerformGet(struct Response *res, struct Request *req)
{
	int rc;
	char *key;
	size_t keylen;

	rc = 0;

	if (req->Uri == NULL) { // special case while booting up?
		return -1;
	}

	// Route: '/'
	if (strlen(req->Uri) == 1 && req->Uri[0] == '/') {
		res->errcode = ERROR_NOKEY;
		return 0;
	}

	key = req->Uri + 1;
	keylen = strlen(key);

	if (KeyIsLegal(key, keylen)) {
		if (KeyDoesExist(key)) {
			res->content = KeyGetData(key);
		} else {
			res->errcode = ERROR_KEYNOTFOUND;
		}
	} else {
		res->errcode = ERROR_ILLEGALKEY;
	}

	return rc;
}

// PerformPost: creates a new file and sets the key in the response
int PerformPost(struct Response *res, struct Request *req)
{
	if (!streq(req->Uri, "/new")) {
		return -1;
	}

	res->content = calloc(1, KEYLEN);
	KeyMake(res->content, KEYLEN);

	return 0;
}

// PerformPut: updates the file at the key, and returns the key
int PerformPut(struct Response *res, struct Request *req)
{
	return 0;
}

// PerformDelete: removes the item at the given key
int PerformDelete(struct Response *res, struct Request *req)
{
	return 0;
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

	GetRequestBody(&req->Body, &req->BodyLen, &req->BodyCap);

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
void GetRequestBody(char **s, size_t *s_len, size_t *s_cap)
{
	int i, c;

	*s = NULL;
	*s_len = *s_cap = 0;

	for (i = 0; (c = getc(FCGI_stdin)) != EOF && i < (1 << 20 << 2); i++) {
		c_resize(s, s_len, s_cap, sizeof(**s));
		(*s)[i] = (char)c;
	}

	if (s && *s && i != 0) {
		(*s)[i] = 0;
	}
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
}

// CleanResponse: cleans up the response object
void CleanResponse(struct Response *res)
{
	if (res) {
		memset(res, 0, sizeof(*res));
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

