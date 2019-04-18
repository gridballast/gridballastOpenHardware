/**
 * @file http_server.h
 *
 * @brief Simple HTTP server for runtime system configuration
 *
 * @author Aaron Perley (aperley@andrew.cmu.edu)
 */

#ifndef __http_server_h_
#define __http_server_h_

/** @brief http client connection state */
typedef struct {
    int fd;
} http_client_conn_t;

/** @brief http request data */
typedef struct {
    unsigned int method;
    char *url;
    char *body;
    bool complete;
} http_req_t;

/**
 * @brief callback to handle http connecton
 *
 * @param client_conn client connection state
 * @param req         request data
 */
typedef void (*http_req_handler_t)(http_client_conn_t *client_conn, http_req_t *req);

void http_server_run(int port, http_req_handler_t req_handler);
int http_client_send(http_client_conn_t *client_conn, const char *buf);

#endif /* __http_server_h_ */

