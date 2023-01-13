#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "steam.h"
#include "toast.h"

#include "commands.h"
#include "updater.h"

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(arr[0])

static int install(int, char**);
static int update(int, char**);
static int run(int, char**);
static int version(int, char**);
static int info(int, char**);

#include "pool.h"

const struct Command commands[] = {
    { .name = "install",   .func = install, .description = "Install OpenFortress"},
    { .name = "update",    .func = update,  .description = "Update an existing install"},
    { .name = "run",       .func = run,     .description = "Run OpenFortress"},
    { .name = "version",   .func = version, .description = "Display the OFCL version"},
    { .name = "info",      .func = info,    .description = "Show info about the current setup"},
};
const size_t commands_size = ARRAY_LEN(commands);

static int install(int c, char** v)
{
    int exit_val = EXIT_SUCCESS;
    char* of_dir = NULL;
    char* remote = NULL;
    for (int i = 1; i < c; ++i)
    {
        if (!strcmp(v[i], "--dir"))
        {
            if (!v[++i])
            {
                puts("No Directory specified");
                return EXIT_FAILURE;
            }
            of_dir = strdup(v[i]);
            printf("Using %s as the directory\n", of_dir);
        }
        else if (!strcmp(v[i], "--remote"))
        {
            if (!v[++i])
            {
                puts("No URL specified");
                return EXIT_FAILURE;
            }
            remote = strdup(v[i]);
            printf("Using %s as the server\n", remote);
        }
        else
        {
            if (strcmp(v[i], "--help"))
                printf("%s is not a valid flag\n\n", v[i]);

            puts(
                "OFCL install\n"
                "\t--dir\t\tspecify where to download OpenFortress to\n"
                "\t--remote\tspecify the server to use\n"
                "\t--help\t\tshows this text"
            );
            return EXIT_FAILURE;
        }
    }
    if (!of_dir)
        of_dir = getOpenFortressDir();

    if (getLocalRevision(of_dir) >= 0)
    {
        puts("OpenFortress is already installed");
        exit_val = EXIT_FAILURE;
        goto install_cleanup;
    }

    if (!remote)
        remote = getLocalRemote(of_dir);

    int remote_rev = getLatestRemoteRevision(remote);

    if (remote_rev != -1)
    {
        update_setup(of_dir, remote, 0, remote_rev);
    }
    else
    {
        puts("Failed to get the latest revision");
        exit_val = EXIT_FAILURE;
    }

    install_cleanup:
    if (remote)
        free(remote);
    free(of_dir);

    return exit_val;
}

static int update(int c, char** v)
{
    int exit_val = EXIT_SUCCESS;
    int verify = 0;
    char* of_dir = NULL;
    char* remote = NULL;
    for (int i = 1; i < c; ++i)
    {
        if (!strcmp(v[i], "--verify"))
            verify = 1;
        else if (!strcmp(v[i], "--dir"))
        {
            if (!v[++i])
            {
                puts("No Directory specified");
                return EXIT_FAILURE;
            }
            of_dir = strdup(v[i]);
            printf("Using %s as the directory\n", of_dir);
        }
        else if (!strcmp(v[i], "--remote"))
        {
            if (!v[++i])
            {
                puts("No URL specified");
                return EXIT_FAILURE;
            }
            remote = strdup(v[i]);
            printf("Using %s as the server\n", remote);
        }
        else
        {
            if (strcmp(v[i], "--help"))
                printf("%s is not a valid flag\n\n", v[i]);

            puts(
                "OFCL update\n"
                "\t--verify\t\tverify game files\n"
                "\t--dir\t\tspecify where to download OpenFortress to\n"
                "\t--remote\tspecify the server to use\n"
                "\t--help\t\tshows this text"
            );
            return EXIT_FAILURE;
        }
    }

    if (!of_dir)
        of_dir = getOpenFortressDir();

    int local_rev = getLocalRevision(of_dir);
    if (verify)
    {
        local_rev = 0;
    }
    else if (0 > local_rev)
    {
        puts("OpenFortress is not installed");
        exit_val = EXIT_FAILURE;
        goto update_cleanup;
    }

    if (!remote)
        remote = getLocalRemote(of_dir);

    if (!strlen(remote))
    {
        puts("Remote is invalid");
        exit_val = EXIT_FAILURE;
        goto update_cleanup;
    }

    int remote_rev = getLatestRemoteRevision(remote);

    if (remote_rev == -1)
    {
        puts("Failed to get the latest revision");
        exit_val = EXIT_FAILURE;
        goto update_cleanup;
    }
    else if (remote_rev <= local_rev)
    {
        puts("Already up to date");
        goto update_cleanup;
    }

    update_setup(of_dir, remote, local_rev, remote_rev);

    update_cleanup:
    if (remote)
        free(remote);
    free(of_dir);
    return exit_val;
}

static int run(int c, char** v)
{
    int exit_val = EXIT_SUCCESS;
    char* of_dir = getOpenFortressDir();

    int (*launch_func)(char**, size_t) = runOpenFortress;

    int arg_index;
    for (arg_index = 1; arg_index < c; ++arg_index)
    {
        if (v[arg_index][0] != '-' && v[arg_index][1] != '-')
            break;

        if (!strcmp(v[arg_index]+2, "direct"))
        {
            
            launch_func = runOpenFortressDirect;
        }
        else if (!strcmp(v[arg_index]+2, "naive"))
        {
            
            launch_func = runOpenFortressNaive;
        }
        else if (!strcmp(v[arg_index]+2, "steam"))
        {
            
            launch_func = runOpenFortressSteam;
        }
        else
        {
            fprintf(stderr,
                "OFCL run [flags] [options]\n"
                "\n"
                " flags:\n"
                "    --direct    launch OpenFortress directly\n"
                "    --naive     tell steam to launch the game\n"
                "    --steam     launch game via Steam\n"
            );
            return 0;
        }
    }


    if (launch_func == runOpenFortressDirect) { fprintf(stderr, "Launching directly\n"); }
    else if (launch_func == runOpenFortressNaive) { fprintf(stderr, "Launching naively\n"); }
    else if (launch_func == runOpenFortressSteam) { fprintf(stderr, "Launching via Steam\n"); }

    int local_rev = getLocalRevision(of_dir);
    if (0 > local_rev)
    {
        puts("OpenFortress is not installed");
        exit_val = EXIT_FAILURE;
        goto run_cleanup;
    }

    if (getSteamPID() == -1)
    {
        puts("Steam is not running");
        exit_val = EXIT_FAILURE;
        goto run_cleanup;
    }

    launch_func(v+1, (size_t)(c-1));

    run_cleanup:
    free(of_dir);
    return exit_val;
}

static int version(int c, char** v)
{
    puts(VERSION);
    return EXIT_SUCCESS;
}

static int info(int c, char** v)
{
    char* of_dir = getOpenFortressDir();
    if (!of_dir)
        of_dir = strdup("Not Found");
    printf("Install Directory:\n\t%s\n", of_dir);

    char* remote = getLocalRemote(of_dir);
    printf("Server:\n\t%s\n", remote);
    free(remote);

    int local_rev = getLocalRevision(of_dir);
    printf("Revision:\t\n");
    if (local_rev == -1)
        puts("\tNot installed");
    else
        printf("\t%i\n", local_rev);

    free(of_dir);

    return EXIT_SUCCESS;
}
