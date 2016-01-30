#include "../../include/tlk_client.h"

/* TODO: add sem for thread safe stdout printing */

int shouldStop = 0;

/* Initialize client data */
tlk_socket_t initialize_client (const char *argv[]) {

  int ret;
  tlk_socket_t socket_desc;
  struct sockaddr_in endpoint_addr = { 0 };

  struct in_addr ip_addr;
  unsigned short port_number;

  /* Parse IP address (default: "127.0.0.1") */
  if (LOG) printf("--> Parse IP address\n");
  ret = inet_pton(AF_INET, argv[1], (void *) &ip_addr);

  if (ret <= 0) {
    if (ret == 0) {
      fprintf(stderr, "Address not valid\n");
    } else if (ret == -1 && errno == EAFNOSUPPORT) {
      fprintf(stderr, "Address family not valid\n");
    }

    exit(EXIT_FAILURE);
  }

  /* Parse port number (default: 3000) */
  if (LOG) printf("--> Parse port number\n");
  ret = parse_port_number(argv[2], &port_number);

  if (ret == -1) {
    fprintf(stderr, "Port not valid: must be between 1024 and 49151\n");

    exit(EXIT_FAILURE);
  }

  /* Create socket */
  if (LOG) printf("--> Create socket\n");
  socket_desc = tlk_socket_create(AF_INET, SOCK_STREAM, 0);
  ERROR_HELPER(socket_desc, "Cannot create socket");

  endpoint_addr.sin_addr = ip_addr;
  endpoint_addr.sin_family = AF_INET;
  endpoint_addr.sin_port = port_number;

  /* Connect to given IP on port */
  if (LOG) printf("--> Connecting to server\n");
  ret = tlk_socket_connect(socket_desc, (const struct sockaddr *) &endpoint_addr, sizeof(struct sockaddr_in));
  ERROR_HELPER(ret, "Cannot connect to endpoint");

  return socket_desc;
}

/* Handle chat session */
void chat_session (tlk_socket_t socket) {

    int ret, exit_code;
    tlk_thread_t chat_threads[2];

    /* Launch receiver thread */
    if (LOG) printf("--> Launch receiver thread\n");
    ret = tlk_thread_create(&chat_threads[0], receiver, (tlk_thread_args) socket);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create receiver thread");

    /* Launch sender thread */
    if (LOG) printf("--> Launch sender thread\n");
    ret = tlk_thread_create(&chat_threads[1], sender, (tlk_thread_args) socket);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create sender thread");

    /* Wait for termination */
    if (LOG) printf("--> Wait for receiver thread termination\n");
    ret = tlk_thread_join(chat_threads[0], &exit_code);
    GENERIC_ERROR_HELPER(ret, exit_code, "Cannot wait for receiver thread termination");

    if (LOG) printf("--> Wait for sender thread termination\n");
    ret = tlk_thread_join(chat_threads[1], &exit_code);
    GENERIC_ERROR_HELPER(ret, exit_code, "Cannot wait for sender thread termination");

    /* Clean resources */
    if (LOG) printf("--> Close socket\n");
    ret = tlk_socket_close(socket);
    ERROR_HELPER(ret, "Cannot close socket");
  }

/* Sender thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI sender (LPVOID arg)
#elif defined(__linux__) && __linux__

void * sender (void *arg)
#endif /* Sender func definition */
{
  if (LOG) printf("\n\t*** [SND] Sender thread running\n\n");

  int ret;
  tlk_socket_t socket = (tlk_socket_t) arg;

  /* Set up close comand */
  if (LOG) printf("\n\t*** [SND] Set up close command\n\n");

  char buf[MSG_SIZE];
  char close_command[MSG_SIZE];

  snprintf(close_command, strlen(COMMAND_CHAR + QUIT_COMMAND), "%c%s\n", COMMAND_CHAR, QUIT_COMMAND);

  size_t close_command_len = strlen(close_command);

  while (!shouldStop)
  {
    /* Read from stdin */
    if (LOG) printf("\n\t*** [SND] Read from stdin\n\n");

/* TODO: implement a prompt function */
    printf("--> ");
    if (fgets(buf, sizeof(buf), stdin) != (char *) buf) {
      fprintf(stderr, ">>> Error reading from stdin, exiting... <<<\n");
      exit(EXIT_FAILURE);
    }

    /* Check if endpoint has closed the connection */
    if (LOG) printf("\n\t*** [SND] Check if endpoint has closed connection\n\n");
    if (shouldStop) break;

    /* Send message through socket */
    if (LOG) printf("\n\t*** [SND] Send message through socket\n\n");
    size_t msg_len = strlen(buf);

    while ( (ret = send(socket, buf, msg_len, 0)) < 0 )
    {

      if (errno == TLK_EINTR) continue;
      ERROR_HELPER(-1, "Cannot write to socket");

    }

    /* Check if message was quit command */
    if (msg_len == close_command_len && strncmp(buf, close_command, close_command_len) == 0) {
      if (LOG) printf("\n\t*** [SND] Message was a quit command\n\n");
      shouldStop = 1;
    }
  }

  /* Terminate sender thread */
  if (LOG) printf("\n\t*** [SND] Sender thread termination\n\n");
  tlk_thread_exit(NULL);
}

/* Receiver thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI receiver (LPVOID arg)
#elif defined(__linux__) && __linux__

void * receiver (void *arg)
#endif /* Receiver func definition */
{
  if (LOG) printf("\n\t*** [REC] Receiver thread running\n\n");

  int ret;
  tlk_socket_t socket = (tlk_socket_t) arg;

  /* Set up close command */
  if (LOG) printf("\n\t*** [REC] Set up close command\n\n");

  char close_command[MSG_SIZE];

  snprintf(close_command, sizeof(char) + strlen(QUIT_COMMAND), "%c%s", COMMAND_CHAR, QUIT_COMMAND);

  size_t close_command_len = strlen(close_command);

  /* Set up timeout interval */
  if (LOG) printf("\n\t*** [REC] Set up timeout interval\n\n");
  struct timeval timeout;
  fd_set read_descriptors;
  int nfds = socket + 1;

  char buf[MSG_SIZE];
  char delimiter[2];

  snprintf(delimiter, 1, "%c", MSG_DELIMITER_CHAR);

  while (!shouldStop) {

    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    FD_ZERO(&read_descriptors);
    FD_SET(socket, &read_descriptors);

    /* Perform select to check the availability of a read desc */
    if (LOG) printf("\n\t*** [REC] Select available read descriptor\n\n");
    ret = select(nfds, &read_descriptors, NULL, NULL, NULL/*&timeout*/);

    if (ret == -1 && errno == TLK_EINTR) continue;
    ERROR_HELPER(ret, "Cannot select read descriptor");

    if (ret == 0) continue; /* timeout expired */

    /* Read when possible */
    int read_completed = 0;
    int read_bytes = 0;
    int bytes_left = MSG_SIZE - 1;

    if (LOG) printf("\n\t*** [REC] Read data\n\n");
    while (!read_completed) {

      ret = recv(socket, buf + read_bytes, 1, 0);

      if (ret == 0) break;
      if (ret == -1 && errno == TLK_EINTR) continue;
      ERROR_HELPER(ret, "Cannot read from socket");

      bytes_left -= ret;
      read_bytes += ret;

      read_completed = bytes_left <= 0 || buf[read_bytes - 1] == '\n';
    }

    if (ret == 0) {
      /* Endpoint has closed unexpectedly */
      if (LOG) printf("\n\t*** [REC] Endpoint returned 0\n\n");
      shouldStop = 1;
    } else {
      /* Print data to user */
      if (LOG) printf("\n\t*** [REC] Show data to user\n\n");
      buf[read_bytes - 1] = '\0';
      printf("[Server] %s\n", buf);
    }
  }

  /* Terminate receiver thread */
  if (LOG) printf("\n\t*** [REC] Receiver thread termination\n\n");
  tlk_thread_exit(NULL);
}

int main (int argc, const char *argv[]) {

  tlk_socket_t socket;

  if (argc != 3) {
    usage_error_client(argv[0]);
  }

  /* Initialize client data */
  if (LOG) printf("\n-> Initialize client data\n\n");
  socket = initialize_client(argv);

  /* Handle chat session */
  if (LOG) printf("\n-> Handle chat session\n\n");
  chat_session(socket);

  /* Close client */
  if (LOG) printf("\n-> Close client\n\n");
  exit(EXIT_SUCCESS);
}