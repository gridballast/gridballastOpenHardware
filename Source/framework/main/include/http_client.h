/**
 * @file http_client.h
 *
 * @brief extremely simple http client
 *
 * @author Aaron Perley (aperley@andrew.cmu.edu)
 */

#ifndef __http_client_h_
#define __http_client_h_

#define CLIENT_BUF_SIZE (4096)
#define STATUS_OK (200)

typedef enum {
    HTTP_CLIENT_GET,
    HTTP_CLIENT_POST
} http_verb_t;

typedef struct {
    char data[CLIENT_BUF_SIZE];
    size_t len;
} http_str_t;

typedef struct {
    http_str_t buf;
    int sock;
    int status_code;
    char has_body;
} http_client_t;


int http_req_begin(http_client_t *client, const char *hostname, int port, http_verb_t verb, const char *uri);
int http_req_add_header(http_client_t *client, const char *header);
int http_req_add_body(http_client_t *client, const char *body, size_t body_len);
int http_req_end(http_client_t *client);

#endif /* __http_client_h_ */
