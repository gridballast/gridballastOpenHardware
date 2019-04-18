/**
 * @file http_server.c
 *
 * @brief Simple HTTP server for runtime system configuration
 *
 * @author Aaron Perley (aperley@andrew.cmu.edu)
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "http_parser.h"
#include "http_server.h"

/** @brief Number of queued clients */
#define LISTEN_QUEUE_LEN (5)

static const char *TAG = "http_server";

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

/**
 * @brief utility function to append to a malloc'd string
 *
 * @param dest pointer to malloc'd string to append to
 * @param src  string to append
 * @param len  length to append
 *
 * @return 0 on success, -1 on failure
 */
static int create_or_append_str(char **dest, const char *src, size_t len) {
    // malloc or realloc
    if (*dest == NULL) {
        // dest has not been allocated
        *dest = malloc(len + 1); // extra byte for null terminator
        if (*dest == NULL) {
            return -1;
        }
        (*dest)[0] = '\0'; // initally null terminate so strncat will work
    } else {
        // dest has been allocated so grow
        char *new_dest = realloc(*dest, strlen(*dest) + len + 1);
        if (new_dest == NULL) {
            free(*dest);
            return -1;
        }
        *dest = new_dest;
    }

    strncat(*dest, src, len);
    return 0;
}


/**
 * @brief callback that runs when the parse is complete
 *
 * Sets complete field of request data so we know to stop reading from client
 */
int on_complete_cb(http_parser *parser) {
    http_req_t *req_data = (http_req_t *)parser->data;
    req_data->complete = true;
    return 0;
}

/**
 * @brief callback that runs when url is parsed
 */
int on_url_cb(http_parser *parser, const char *at, size_t length) {
    http_req_t *req_data = (http_req_t *)parser->data;
    req_data->method = parser->method;
    return create_or_append_str(&req_data->url, at, length);
}

/**
 * @brief callback that runs when request body is received
 */
int on_body_cb(http_parser *parser, const char *at, size_t length) {
    http_req_t *req_data = (http_req_t *)parser->data;
    return create_or_append_str(&req_data->body, at, length);
}

/**
 * @brief parse http client request and trigger user callback
 * 
 * @param fd socket file descriptor
 */
void handle_client(http_client_conn_t *conn, http_req_handler_t req_handler) {
    const size_t buf_len = 1024;
    char *buf = malloc(buf_len);
    if (buf == NULL) {
        return;
    }

    // configure http parser
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_complete = on_complete_cb;
    settings.on_url = on_url_cb;
    settings.on_body = on_body_cb;

    http_req_t *req_data = malloc(sizeof(http_req_t));
    if (req_data == NULL) {
        free(buf);
        return;
    }
    memset(req_data, 0, sizeof(http_req_t));
    parser.data = req_data;

    // read and parse request
    bool read_err = false;
    while (!req_data->complete && HTTP_PARSER_ERRNO(&parser) == HPE_OK) {
        ssize_t read_len = read(conn->fd, buf, buf_len);
        if (read_len < 0) {
            read_err = true;
            ESP_LOGE(TAG, "Socket read error: %s", strerror(errno));
            break;
        }
        
        int nparsed = http_parser_execute(&parser, &settings, buf, read_len);
        if (nparsed != read_len) {
            read_err = true;
            ESP_LOGE(TAG, "HTTP parser error: read %zd bytes but parsed %d bytes",
                     read_len, nparsed);
            break;
        }
    }

    free(buf);

    if (!read_err) {
        enum http_errno http_errno = HTTP_PARSER_ERRNO(&parser);
        if (http_errno == HPE_OK) {
            // let user handle request
            req_handler(conn, req_data);
        } else {
            ESP_LOGE(TAG, "HTTP parser error: %s %s", http_errno_name(http_errno),
                                                      http_errno_description(http_errno));
        }
    }

    if (req_data->url != NULL) {
        free(req_data->url);
    }
    if (req_data->body != NULL) {
        free(req_data->body);
    }
    free(req_data);
}


/*****************************************
 *********** INTERFACE FUNCTIONS *********
 *****************************************/

/**
 * @brief run http server
 *
 * @param port        port to listen on
 * @param req_handler user callback function to run on parsed request
 */
void http_server_run(int port, http_req_handler_t req_handler) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size = sizeof(client_addr);
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    int server_sock;
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ESP_LOGE(TAG, "bind error %s", strerror(errno));
    }
    if (bind(server_sock, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) < 0) {
        ESP_LOGE(TAG, "bind error %s", strerror(errno));
    }
    if (listen(server_sock, LISTEN_QUEUE_LEN) < 0) {
        ESP_LOGE(TAG, "listen error %s", strerror(errno));
    }

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &sin_size);
        if (client_sock < 0) {
            ESP_LOGE(TAG, "accept error %s", strerror(errno));
            break;
        }
        http_client_conn_t client_conn;
        client_conn.fd = client_sock;
        handle_client(&client_conn, req_handler);
        close(client_sock);
    }
}

const char *http_resp_header_fmt = 
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n"
    "Conent-Length: %zu\r\n"
    "\r\n";

/**
 * @brief send response data back to client
 *
 * @param client_conn client connection state
 * @param buf         response body
 *
 * @note This function must only be called once from inside the user request
 *       handler callback. It only supports 200 OK responses right now.
 */
int http_client_send(http_client_conn_t *client_conn, const char *buf) {
    int header_len = snprintf(NULL, 0, http_resp_header_fmt, strlen(buf));
    char *header_buf = malloc(header_len + 1);
    if (header_buf == NULL) {
        return -1;
    }
    snprintf(header_buf, header_len + 1, http_resp_header_fmt, strlen(buf));
    ssize_t write_len = write(client_conn->fd, header_buf, header_len);
    if (write_len < 0) {
        free(header_buf);
        return -1;
    }
    free(header_buf);

    write_len = write(client_conn->fd, buf, strlen(buf));
    if (write_len < 0) {
        return -1;
    }
    return 0;
}
