// -----------------------------------
// CSCI 340 - Operating Systems
// Spring 2025
// server.c source file
// Assignment 2
//
// -----------------------------------

#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"

// ------------------------------------
// Function prototype that creates a socket and 
// then binds it to the specified port_number 
// for incoming client connections
// 
// Arguments:	port_number = port number server socket will be bound to.
//
// Return:      Socket file descriptor (or -1 on failure)
//
int bind_port(unsigned int port_number)
{
  // -------------------------------------
  // NOTHING TODO HERE :)
  // -------------------------------------
  // DO NOT MODIFY !

  int socket_fd;
  int set_option = 1;

  struct sockaddr_in server_address;
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&set_option,
	     sizeof(set_option));

  if (socket_fd < 0) return FAIL;

  bzero((char *) &server_address, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_number);

  if (bind(socket_fd, (struct sockaddr *) &server_address,
	     sizeof(server_address)) == 0) {
    return socket_fd;
  } else {
    return FAIL;
  }
} // end bind_port function

// ------------------------------------
// Function prototype that accepts a client
// socket connection
// 
// Arguments:	server file descriptor
//
// Return:      Termination status of client (0 = No Errors, -1 = Error)
//
int accept_client(int server_socket_fd)
{
    printf("accept_client() function called\n");
    int exit_status = OK;
    int client_socket_fd = -1;
    socklen_t client_length; 
    struct sockaddr_in client_address;
    char request[2048];
    client_length = sizeof(client_address);

    // Accept connection
    client_socket_fd = accept(server_socket_fd,
                              (struct sockaddr *) &client_address,
                              &client_length);
    if (client_socket_fd < 0) {
        perror("accept() error");
        return FAIL;
    }

    // Fork a child to handle the request
    int pid = fork();
    if (pid > 0) {
        // --------------------------
        // PARENT PROCESS
        // --------------------------
        // Immediately close the child’s socket so the parent can
        // keep listening for new connections.
        close(client_socket_fd);
        return OK;
    }
    else if (pid < 0) {
        // Fork error
        perror("fork() error");
        close(client_socket_fd);
        return FAIL;
    }

    // --------------------------
    // CHILD PROCESS
    // --------------------------
    // The child does not need the original server socket
    close(server_socket_fd);

    // Read the raw HTTP request into 'request'
    bzero(request, sizeof(request));
    read(client_socket_fd, request, sizeof(request) - 1);

    // 1) Parse out the METHOD, PATH, and VERSION from the request line
    //    Example request line:
    //    GET /?first=anthony&last=leclerc HTTP/1.1
    char method[8], path[1024], version[16];
    bzero(method, sizeof(method));
    bzero(path, sizeof(path));
    bzero(version, sizeof(version));

    // Use sscanf to grab the first line (method, path, version)
    sscanf(request, "%s %s %s", method, path, version);

    //
    // Check if client requested /SimplePost.html with GET
    //
    // If so, we can serve a small hard-coded HTML page (or read from disk).
    //
    if (strncmp(method, "GET", 3) == 0 && strstr(path, "SimplePost.html")) {
        // SimplePost.html:
        const char *simple_post_html =
            "<html>\r\n"
            "<head>\r\n"
            "  <meta charset=\"UTF-8\">\r\n"
            "  <title>Simple Post</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "  <form method=\"POST\" action=\"/\">\r\n"
            "    <b>first</b>&nbsp;<input type=\"text\" name=\"first\"><br>\r\n"
            "    <b>last</b>&nbsp;<input type=\"text\" name=\"last\"><br>\r\n"
            "    <input type=\"submit\">\r\n"
            "  </form>\r\n"
            "</body>\r\n"
            "</html>\r\n";

        char response[4096];
        sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: %d\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "%s",
                (int)strlen(simple_post_html),
                simple_post_html);

        write(client_socket_fd, response, strlen(response));
        close(client_socket_fd);
        exit(OK);
    }

    // 2) We will store our response HTML in here:
    char entity_body[8192];
    bzero(entity_body, sizeof(entity_body));

    // Simple helper function
    #define APPEND_STR(dst, src) strncat(dst, src, sizeof(dst) - strlen(dst) - 1)

    // Prepare the initial HTML
    // We'll fill in either "GET Operation" or "POST Operation"
    // plus the table rows for key=value pairs
    if (strncmp(method, "GET", 3) == 0) {
        // -------------------------------------------------
        //   PARSE GET
        // -------------------------------------------------
        APPEND_STR(entity_body, "<html><body>\n");
        APPEND_STR(entity_body, "<h1>GET Operation</h1>\n");
        APPEND_STR(entity_body, "<table cellpadding=5 cellspacing=5 border=1>\n");

        // Look for '?' in the path
        char *qmark = strchr(path, '?');
        if (qmark != NULL) {
            // skip the '?'
            qmark++;

            // Tokenize on '&'
            char *pair = strtok(qmark, "&");
            while (pair) {
                // each pair is "key=value"
                char *eq = strchr(pair, '=');
                if (eq) {
                    *eq = '\0';   // split pair into key / value
                    char *key = pair;
                    char *val = eq + 1;

                    APPEND_STR(entity_body, "<tr><td><b>");
                    APPEND_STR(entity_body, key);
                    APPEND_STR(entity_body, "</b></td><td>");
                    APPEND_STR(entity_body, val);
                    APPEND_STR(entity_body, "</td></tr>\n");
                }
                pair = strtok(NULL, "&");
            }
        }

        APPEND_STR(entity_body, "</table>\n</body></html>\n");
    }
    else if (strncmp(method, "POST", 4) == 0) {
        // -------------------------------------------------
        //   PARSE POST
        // -------------------------------------------------
        APPEND_STR(entity_body, "<html><body>\n");
        APPEND_STR(entity_body, "<h1>POST Operation</h1>\n");
        APPEND_STR(entity_body, "<table cellpadding=5 cellspacing=5 border=1>\n");

        // For POST, the key=value pairs are in the *body* after the blank line
        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4; // skip the "\r\n\r\n"

            // Body should now point to "first=anthony&last=leclerc" etc.
            char *pair = strtok(body, "&");
            while (pair) {
                char *eq = strchr(pair, '=');
                if (eq) {
                    *eq = '\0';
                    char *key = pair;
                    char *val = eq + 1;
                    APPEND_STR(entity_body, "<tr><td><b>");
                    APPEND_STR(entity_body, key);
                    APPEND_STR(entity_body, "</b></td><td>");
                    APPEND_STR(entity_body, val);
                    APPEND_STR(entity_body, "</td></tr>\n");
                }
                pair = strtok(NULL, "&");
            }
        }

        APPEND_STR(entity_body, "</table>\n</body></html>\n");
    }
    else {
        // -------------------------------------------------
        //   UNSUPPORTED METHOD
        // -------------------------------------------------
        strcpy(entity_body, "<html><body><h1>501 Not Implemented</h1></body></html>");
    }

    // 3) Build the full HTTP response (status line + headers + blank line + body)
    char response[16384];
    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "%s",
            (int)strlen(entity_body),
            entity_body);

    // 4) Send it back
    write(client_socket_fd, response, strlen(response));
    close(client_socket_fd);
    exit(OK);
}
