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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <fcgi_stdio.h>

char *ENVVARS[] = {
    "GATEWAY_INTERFACE",
    "SERVER_SOFTWARE"
    "QUERY_STRING",
    "DOCUMENT_ROOT",
    "HTTP_COOKIE",
    "HTTP_HOST",
    "HTTP_REFERER",
    "HTTP_USER_AGENT",
    "HTTPS",
    "PATH",
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
};

enum {
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_UNDEFINED,
	METHOD_TOTAL
};

// WebRequest: a structure to hold everything needed for the web request
struct WebRequest {
	int Method;

	char *Uri;

	char *Body;
	size_t BodyLen;
	size_t BodyCap;
};

// printallenv : prints all of the env vars
void printallenv(void)
{
	char *format;
	char *s;
	int i;

	format = "<p>%s : <strong>%s</strong></p>";

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
int ParseRequest(struct WebRequest *wr);
// ParseMethod: returns the enum for the request method
int ParseMethod(char *request);
// GetRequestBody: gets the request body, if there is one
void GetRequestBody(char **s, size_t *s_len, size_t *s_cap);

// Response Methods
// SendResponse: sends the response
int SendResponse(struct WebRequest *wr);

// Helper Functions
// Bootstrap: bootstraps the program
void Bootstrap(void);
// CleanRequest: cleans up the request object
void CleanRequest(struct WebRequest *wr);

int main(int argc, char **argv)
{
	struct WebRequest wr;
	int rc;

	Bootstrap();

	while (FCGI_Accept() >= 0) {
		rc = ParseRequest(&wr);
		if (rc < 0) {
			printf("ERROR!");
		}

		SendResponse(&wr);
		if (rc < 0) {
			printf("ERROR!");
		}

		CleanRequest(&wr);
	}

	return 0;
}

// SendResponse: sends the response
int SendResponse(struct WebRequest *wr)
{
	printf("Content-type: text/plain\r\n");

	printf("\r\n");

	printf("Hello World!");

	if (wr->Body)
		printf("%s", wr->Body);

	return 0;
}

// ParseRequest: returns a request structure
int ParseRequest(struct WebRequest *wr)
{
	memset(wr, 0, sizeof(*wr));

	if (getenv("REQUEST_METHOD") != NULL) {
		wr->Method = ParseMethod(getenv("REQUEST_METHOD"));
	}

	if (getenv("REQUEST_URI") != NULL) {
		wr->Uri = strdup(getenv("REQUEST_URI"));
	}

	GetRequestBody(&wr->Body, &wr->BodyLen, &wr->BodyCap);

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
	rc = mkdir("./data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (rc < 0) {
		perror(strerror(rc));
	}
}

// CleanRequest: cleans up the request object
void CleanRequest(struct WebRequest *wr)
{
	if (wr) {
		free(wr->Uri);
		free(wr->Body);

		memset(wr, 0, sizeof(*wr));
	}
}

