/*
 * Copyright 6WIND S.A., 2014
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"

#include "ivshmem-client.h"

#define IVSHMEM_CLIENT_DEFAULT_VERBOSE        0
#define IVSHMEM_CLIENT_DEFAULT_UNIX_SOCK_PATH "/tmp/ivshmem_socket"

int
main(int argc, char *argv[])
{
    struct sigaction sa;
    IvshmemClient client;
    IvshmemClientArgs args = {
        .verbose = IVSHMEM_CLIENT_DEFAULT_VERBOSE,
        .unix_sock_path = IVSHMEM_CLIENT_DEFAULT_UNIX_SOCK_PATH,
    };

    /* parse arguments, will exit on error */
    ivshmem_client_parse_args(&args, argc, argv);

    /* Ignore SIGPIPE, see this link for more info:
     * http://www.mail-archive.com/libevent-users@monkey.org/msg01606.html */
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 ||
        sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE; sigaction");
        return 1;
    }

    ivshmem_client_cmdline_help();
    printf("cmd> ");
    fflush(stdout);

    if (ivshmem_client_init(&client, args.unix_sock_path,
                            ivshmem_client_notification_cb, NULL,
                            args.verbose) < 0) {
        fprintf(stderr, "cannot init client\n");
        return 1;
    }

    while (1) {
        if (ivshmem_client_connect(&client) < 0) {
            fprintf(stderr, "cannot connect to server, retry in 1 second\n");
            sleep(1);
            continue;
        }

        fprintf(stdout, "listen on server socket %d\n", client.sock_fd);

        if (ivshmem_client_poll_events(&client) == 0) {
            continue;
        }

        /* disconnected from server, reset all peers */
        fprintf(stdout, "disconnected from server\n");

        ivshmem_client_close(&client);
    }

    return 0;
}
