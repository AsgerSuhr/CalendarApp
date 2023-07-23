/* Custom code that needs to be added to http_client.c in the LWIP library */

/* GET request with host */
#define HTTPC_REQ_11_HOST_HA_API "GET %s HTTP/1.1\r\n" /* URI */\
    "User-Agent: %s\r\n" /* User-Agent */ \
    "Authorization: Bearer %s\r\n" /* Authorization */ \
    "Content-Type: application/json\r\n" /* Content-Type */ \
    "Accept: */*\r\n" \
    "Host: %s\r\n" /* server name */ \
    "Connection: Close\r\n" /* we don't support persistent connections, yet */ \
    "\r\n"
#define HTTPC_REQ_11_HOST_HA_API_FORMAT(uri, auth, srv_name) HTTPC_REQ_11_HOST_HA_API, uri, HTTPC_CLIENT_AGENT, auth, srv_name


static int
httpc_create_request_string_HA(const httpc_connection_t *settings, const char* server_name, int server_port, const char* uri, const char* auth,
                            int use_host, char *buffer, size_t buffer_size)
{
  if (settings->use_proxy) {
    LWIP_ASSERT("server_name != NULL", server_name != NULL);
    if (server_port != HTTP_DEFAULT_PORT) {
      return snprintf(buffer, buffer_size, HTTPC_REQ_11_PROXY_PORT_FORMAT(server_name, server_port, uri, server_name));
    } else {
      return snprintf(buffer, buffer_size, HTTPC_REQ_11_PROXY_FORMAT(server_name, uri, server_name));
    }
  } else if (use_host) {
    LWIP_ASSERT("server_name != NULL", server_name != NULL);
    int res = snprintf(buffer, buffer_size, HTTPC_REQ_11_HOST_HA_API_FORMAT(uri, auth, server_name));
    printf("%s", buffer);
    return res;
  } else {
    return snprintf(buffer, buffer_size, HTTPC_REQ_11_FORMAT(uri));
  }
}


/**
 * Initialize the connection struct HA
 */
static err_t
httpc_init_connection_HA(httpc_state_t **connection, const httpc_connection_t *settings, const char* server_name,
                      u16_t server_port, const char* uri, const char* auth, altcp_recv_fn recv_fn, void* callback_arg)
{
  return httpc_init_connection_common_HA(connection, settings, server_name, server_port, uri, auth, recv_fn, callback_arg, 1);
}


/**
 * Initialize the connection struct (from IP address)
 */
static err_t
httpc_init_connection_addr_HA(httpc_state_t **connection, const httpc_connection_t *settings,
                           const ip_addr_t* server_addr, u16_t server_port, const char* uri, const char* auth,
                           altcp_recv_fn recv_fn, void* callback_arg)
{
  char *server_addr_str = ipaddr_ntoa(server_addr);
  if (server_addr_str == NULL) {
    return ERR_VAL;
  }
  return httpc_init_connection_common_HA(connection, settings, server_addr_str, server_port, uri, auth, recv_fn, callback_arg, 1);
}


/**
 * @ingroup httpc
 * HTTP client API: get a file by passing server name as string (DNS name or IP address string)
 *
 * @param server_name server name as string (DNS name or IP address string)
 * @param port tcp port of the server
 * @param uri uri to get from the server, remember leading "/"!
 * @param settings connection settings (callbacks, proxy, etc.)
 * @param recv_fn the http body (not the headers) are passed to this callback
 * @param callback_arg argument passed to all the callbacks
 * @param connection retreives the connection handle (to match in callbacks)
 * @return ERR_OK if starting the request succeeds (callback_fn will be called later)
 *         or an error code
 */
err_t
httpc_get_file_HA(const char* server_name, u16_t port, const char* uri, const char* auth, const httpc_connection_t *settings,
                   altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection)
{
  err_t err;
  httpc_state_t* req;

  LWIP_ERROR("invalid parameters", (server_name != NULL) && (uri != NULL) && (recv_fn != NULL), return ERR_ARG;);

  err = httpc_init_connection_HA(&req, settings, server_name, port, uri, auth, recv_fn, callback_arg);
  if (err != ERR_OK) {
    return err;
  }

  if (settings->use_proxy) {
    err = httpc_get_internal_addr(req, &settings->proxy_addr);
  } else {
    err = httpc_get_internal_dns(req, server_name);
  }
  if(err != ERR_OK) {
    httpc_free_state(req);
    return err;
  }

  if (connection != NULL) {
    *connection = req;
  }
  return ERR_OK;
}