#include <errno.h>
#include <glib.h>
#include <nss.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define SUFFIX ".mac"

#define ALIGN(idx) do { \
  if (idx % sizeof(void*)) \
    /* Align on word boundary */ \
    idx += (sizeof(void*) - idx % sizeof(void*)); \
} while(0)

static gboolean
lookup_mdns_ip(
    const char *name,
    struct in_addr *addr)
{
    char *stdout_s, *stderr_s, *name_s, **parts;
    int exit_status;
    gboolean success = FALSE;

    /* remove the suffix */
    name_s = g_strdup(name);
    *strrchr(name_s, '.') = '\0';

    char *argv[] = {"avahi-resolve", "-n", name_s, NULL};
    char *envp[] = {"LC_ALL", "C", NULL};

    if (!g_spawn_sync(
        NULL,                 // const gchar *working_directory,
        argv,                 // gchar **argv,
        envp,                 // gchar **envp,
        G_SPAWN_SEARCH_PATH,  // GSpawnFlags flags,
        NULL,                 // GSpawnChildSetupFunc child_setup,
        NULL,                 // gpointer user_data,
        &stdout_s,            // gchar **standard_output,
        &stderr_s,            // gchar **standard_error,
        &exit_status,         // gint *exit_status,
        NULL)) {              // GError **error);
        goto finally;
    }

    // avahi-resolve does not handle (or use) exit codes correctly.
    // This is bad for checking, so we need to search for a failure
    // on the stderr output.
    if (!stderr_s || g_str_has_prefix(stderr_s, "Failed")) {
        goto finally;
    }

    // It's safer to check also for the exit code of avahi-resolve.
    // Just in case they are more compatible to the regular behavior
    // on errors.
    if (exit_status != 0) {
        goto finally;
    }

    // The successful output looks like this:
    //
    //   pdf-api.local	172.17.0.4
    //                 ^--- thats a tab, in between
    //
    // So we split the tab here, and use the last part as our raw ip.
    parts = g_strsplit(stdout_s, "\t", -1);

    // When we have not enough parts, we drop the request.
    if (!parts[1]) {
        goto finally;
    }

    // Parse the last part (assumed ip part) as an IPv4 address.
    stdout_s = g_strstrip(parts[1]);
    success = inet_aton(stdout_s, addr);

finally:

    // EPA loves us
    g_free(name_s);
    g_free(stderr_s);
    g_free(stdout_s);

    return success;
}

static void
pack_hostent(
    struct hostent *result,
    char *buffer,
    size_t buflen,
    const char *name,
    const void *addr)
{
    size_t idx, astart;

    // Alias names, assuming buffer starts a nicely aligned offset
    *((char**) buffer) = NULL;
    result->h_aliases = (char**) buffer;
    idx = sizeof(char*);

    // Official name
    strcpy(buffer+idx, name);
    result->h_name = buffer+idx;
    idx += strlen(name)+1;

    result->h_addrtype = AF_INET;
    result->h_length = sizeof (struct in_addr);

    // Address
    astart = idx;
    memcpy(buffer+astart, addr, result->h_length);
    idx += result->h_length;

    // Address array, idx might not be at pointer alignment anymore,
    // so we need to ensure it is
    ALIGN(idx);

    ((char**) (buffer+idx))[0] = buffer+astart;
    ((char**) (buffer+idx))[1] = NULL;
    result->h_addr_list = (char**) (buffer+idx);
}

enum nss_status
_nss_mac_mdns_gethostbyname2_r(
    const char *name,
    int af,
    struct hostent *result,
    char *buffer,
    size_t buflen,
    int *errnop,
    int *h_errnop)
{
    struct in_addr addr;

    // We just handle IPv4 requests
    if (af != AF_INET) {
        *errnop = EAFNOSUPPORT;
        *h_errnop = NO_DATA;
        return NSS_STATUS_UNAVAIL;
    }

    // We just handle addresses with our defined suffix
    if (!g_str_has_suffix(name, SUFFIX)) {
        *errnop = ENOENT;
        *h_errnop = HOST_NOT_FOUND;
        return NSS_STATUS_NOTFOUND;
    }

    // When the lookup failed, we tell NSS so
    if (!lookup_mdns_ip(name, &addr)) {
        *errnop = ENOENT;
        *h_errnop = HOST_NOT_FOUND;
        return NSS_STATUS_NOTFOUND;
    }

    // Assemble the NSS hostent result
    pack_hostent(result, buffer, buflen, name, &addr);

    return NSS_STATUS_SUCCESS;
}

enum nss_status
_nss_mac_mdns_gethostbyname_r(
    const char *name,
    struct hostent *result,
    char *buffer,
    size_t buflen,
    int *errnop,
    int *h_errnop)
{
    return _nss_mac_mdns_gethostbyname2_r(
        name,
        AF_INET,
        result,
        buffer,
        buflen,
        errnop,
        h_errnop);
}

enum nss_status
_nss_mac_mdns_gethostbyaddr_r(
    const void *addr,
    socklen_t len,
    int af,
    struct hostent *result,
    char *buffer,
    size_t buflen,
    int *errnop,
    int *h_errnop)
{
    // We just handle IPv4 requests
    if (af != AF_INET) {
        *errnop = EAFNOSUPPORT;
        *h_errnop = NO_DATA;
        return NSS_STATUS_UNAVAIL;
    }

    // We need space for the address
    if (len != sizeof (struct in_addr)) {
        *errnop = EINVAL;
        *h_errnop = NO_RECOVERY;
        return NSS_STATUS_UNAVAIL;
    }

    // We do not care of reverse lookups
    *errnop = EAFNOSUPPORT;
    *h_errnop = NO_DATA;
    return NSS_STATUS_UNAVAIL;
}
