/* Custom code that needs to be added to http_client.c in the LWIP library */

err_t httpc_get_file_HA_IP(const ip_addr_t* server_addr, u16_t port, const char* uri, const char* auth, const httpc_connection_t *settings,
               altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection);
err_t httpc_get_file_HA(const char* server_name, u16_t port, const char* uri, const char* auth, const httpc_connection_t *settings,
                   altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection);
