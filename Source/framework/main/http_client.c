/**
 * @file http_client.c
 *
 * @brief extremely simple http client
 *
 * @author Aaron Perley (aperley@andrew.cmu.edu)
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "esp_log.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "http_client.h"
#include "http_parser.h"

static const char * const TAG = "http_client";
static const char * const CRLF = "\r\n";
static char recv_buf[64];

static int get_socket(const char *hostname, const char *port);
static int receive_response(http_client_t *client);
static int receive_on_body(http_parser *parser, const char *buf, size_t len);
static void buf_init(http_client_t *client);
static int buf_append(http_client_t *client, const char *new_str, size_t new_len);
static int buf_flush(http_client_t *client);

/**
 * @brief begin an http request by opening a socket connection and sending initial headers
 *
 * @param client   pointer to http client struct
 * @param hostname name or ip address to connect to
 * @param port     port to connect to
 * @param verb     http verb to use (GET or POST)
 * @param uri      uri to fetch
 *
 * @return 0 on success, -1 on failure
 */
int http_req_begin(http_client_t *client, const char *hostname, int port, http_verb_t verb, const char *uri) {
    // build string buffer from port to pass to getaddrinfo
    int port_str_len = snprintf(NULL, 0, "%d", port);
    char port_str[port_str_len + 1];
    sprintf(port_str, "%d", port);
    
    client->status_code = -1;
    client->has_body = 0;
    buf_init(client);
    client->sock = get_socket(hostname, port_str);
    if (client->sock < 0) {
        return -1;
    }


    char *verb_str;
    if (verb == HTTP_CLIENT_GET) {
        verb_str = "GET ";
    } else {
        verb_str = "POST ";
    }

    // <VERB> <URI> HTTP/1.1
    int ret = buf_append(client, verb_str, strlen(verb_str));
    if (ret >= 0) {
        ret = buf_append(client, uri, strlen(uri));
    }
    if (ret >= 0) {
        const char * const str = " HTTP/1.1\r\n";
        ret = buf_append(client, str, strlen(str));
    }
    
    // Host: <HOSTNAME>:<PORT>\r\n
    if (ret >= 0) {
        const char * const str = "Host: ";
        ret = buf_append(client, str, strlen(str));
    }
    if (ret >= 0) {
        ret = buf_append(client, hostname, strlen(hostname));
    }
    if (ret >= 0) {
        const char * const str = ":";
        ret = buf_append(client, str, strlen(str));
    }
    if (ret >= 0) {
        ret = buf_append(client, port_str, strlen(port_str));
    }
    if (ret >= 0) {
        ret = buf_append(client, CRLF, strlen(CRLF));
    }

    if (ret < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(client->sock);
        return -1;
    }
    
    return 0;
}

/**
 * @brief add a header to an open http request
 *
 * @param client pointer to http client struct
 * @param header string containing header (Header-Name: value)
 *
 * @return 0 on success, -1 on failure
 */
int http_req_add_header(http_client_t *client, const char *header) {
    int ret = buf_append(client, header, strlen(header));
    if (ret >= 0) {
        ret = buf_append(client, CRLF, strlen(CRLF));
    }

    if (ret < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(client->sock);
        return -1;
    }

    return 0;
}

/**
 * @brief add a body to the http request
 *
 * @param client   pointer to http client struct
 * @param body     data to send in the request body
 * @param body_len length of body (excluding NULL terminator if it is a string)
 *
 * @return 0 on success, -1 on failure
 * @note no headers can be added after this function is called
 */
int http_req_add_body(http_client_t *client, const char *body, size_t body_len) {
    // build content-length header
    const char * const content_len_fmt = "Content-Length: %u";
    int header_len = snprintf(NULL, 0, content_len_fmt, body_len); // calculate size
    char header_buf[header_len + 1];
    sprintf(header_buf, content_len_fmt, body_len);
    int ret = http_req_add_header(client, header_buf);

    // write second CRLF
    if (ret >= 0) {
        ret = buf_append(client, CRLF, strlen(CRLF));
    }

    // write body
    if (ret >= 0) {
        ret = buf_append(client, body, body_len);
    }

    client->has_body = 1;

    return 0;
}

/**
 * @brief send the http request and wait for a response
 *
 * @param client pointer to http client struct
 *
 * @return 0 on success, -1 on failure
 */
int http_req_end(http_client_t *client) {
    int ret = 0;
    if (client->has_body == 0) {
        ret = buf_append(client, CRLF, strlen(CRLF));
    }
    if (ret >= 0) {
        ret = buf_flush(client);
    }
    if (ret >= 0) {
        ret = receive_response(client);
    }

    if (ret < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(client->sock);
        return -1;
    }

    return 0;
}


/**
 * @brief perform DNS lookup, allocate a socket, and connect to a host
 *
 * @param hostname name or ip address to connect to
 * @param port     port to connect to
 *
 * @return 0 on success, -1 on failure
 */
static int get_socket(const char *hostname, const char *port) {
    // DNS lookup
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;

    int err = getaddrinfo(hostname, port, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        return -1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "... failed to allocate socket");
        freeaddrinfo(res);
        return -1;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    ESP_LOGI(TAG, "... connected");
    return sock;
}

/**
 * @brief receive an HTTP response from an open client connection
 *
 * @param client   pointer to http client struct
 * @return 0 on success, -1 on failure
 * @note This function currently only saves the return code
 */
static int receive_response(http_client_t *client) {
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0) {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(client->sock);
        return -1;
    }

    // just receiving the response code for now
    int recv_len;
    buf_init(client);

    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = client;
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings)); // set all callbacks to NULL
    settings.on_body = receive_on_body;
    do {
        recv_len = read(client->sock, recv_buf, sizeof(recv_buf));
        if (recv_len < 0) {
            ESP_LOGE(TAG, "... error receiving");
            printf("errno: %s\n", strerror(errno));
            close(client->sock);
            return -1;
        }

        size_t parsed_len = http_parser_execute(&parser, &settings, recv_buf, recv_len);
        if (parsed_len != recv_len || parser.http_errno != HPE_OK) {
            ESP_LOGE(TAG, "response parse error: %s: %s", http_errno_name(parser.http_errno),
                     http_errno_description(parser.http_errno));
            close(client->sock);
            return -1;
        }
    } while (recv_len > 0);

    client->status_code = parser.status_code;
    client->buf.data[client->buf.len] = '\0';
    close(client->sock);
    return 0;
}

/**
 * @brief http parser on_body callback. This is called when the body is parsed
 * and may be called multiple times if the body is split up between recv calls
 */
static int receive_on_body(http_parser *parser, const char *buf, size_t len) {
    http_client_t *client = parser->data;
    size_t copied_len;
    if (len <= sizeof(client->buf.data) - client->buf.len) {
        copied_len = len;
    } else {
        copied_len = sizeof(client->buf.data) - client->buf.len;
    }
    memcpy(client->buf.data + client->buf.len, buf, copied_len);
    client->buf.len += copied_len;
    return 0; // indicate success so parser continues parsing
}

/**
 * @ brief Initialize the client send/receive buffer
 *
 * @param client the client struct
 */
static void buf_init(http_client_t *client) {
    client->buf.len = 0;
}

/**
 * @brief Append a string to the client send/receive buffer
 *
 * @param client  the client struct
 * @param new_str string or data to append
 * @param new_len length of data to append from new_str
 *
 * return 0 on success, -1 on error
 */
static int buf_append(http_client_t *client, const char *new_str, size_t new_len) {
    while (new_len > 0) {
        size_t copied_len;
        if (new_len <= sizeof(client->buf.data) - client->buf.len) {
            copied_len = new_len;
        } else {
            copied_len = sizeof(client->buf.data) - client->buf.len;
        }
        memcpy(client->buf.data + client->buf.len, new_str, copied_len);
        client->buf.len += copied_len;
        new_str += copied_len;
        new_len -= copied_len;

        if (client->buf.len == sizeof(client->buf.data)) {
            if (buf_flush(client) == 0) {
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief flush any data remaining in the buffer to the socket
 *
 * @param client the http client
 * @return 0 on success, -1 on failure
 */
static int buf_flush(http_client_t *client) {
    int ret = write(client->sock, client->buf.data, client->buf.len);
    if (ret >= 0) {
        client->buf.len = 0;
    }
    return ret;
}

