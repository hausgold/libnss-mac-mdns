#include <errno.h>
#include <glib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define EXPECTED_IP "172.17.0.17"

static void
test_gethostbyname(void)
{
    struct hostent *results;
    char buffer[INET_ADDRSTRLEN];

    results = gethostbyname("pdf-api.local.mac");

    g_assert(results != NULL);

    g_assert_cmpstr(results->h_name, ==, "pdf-api.local.mac");
    g_assert(results->h_aliases[0] == NULL);
    g_assert_cmpint(results->h_addrtype, ==, AF_INET);
    g_assert_cmpint(results->h_length, ==, 4);
    g_assert(results->h_addr_list[0] != NULL);
    g_assert(results->h_addr_list[1] == NULL);

    inet_ntop(AF_INET, results->h_addr_list[0], buffer, INET_ADDRSTRLEN);
    g_assert_cmpstr(buffer, ==, EXPECTED_IP);
}

static void
test_gethostbyname_no_suffix(void)
{
    struct hostent *results;

    results = gethostbyname("test.local");

    g_assert(results == NULL);
    g_assert_cmpint(h_errno, ==, HOST_NOT_FOUND);
}

static void
test_gethostbyname_unknown_name(void)
{
    struct hostent *results;

    results = gethostbyname("unknown.local.mac");

    g_assert(results == NULL);
    g_assert_cmpint(h_errno, ==, HOST_NOT_FOUND);
}

static void
test_gethostbyname_exit_code(void)
{
    struct hostent *results;

    results = gethostbyname("exitcode.local.mac");

    g_assert(results == NULL);
    g_assert_cmpint(h_errno, ==, HOST_NOT_FOUND);
}

static void
test_gethostbyname2 (void)
{
    struct hostent *results;
    char buffer[INET_ADDRSTRLEN];

    results = gethostbyname2("pdf-api.local.mac", AF_INET);

    g_assert(results != NULL);

    g_assert_cmpstr(results->h_name, ==, "pdf-api.local.mac");
    g_assert(results->h_aliases[0] == NULL);
    g_assert_cmpint(results->h_addrtype, ==, AF_INET);
    g_assert_cmpint(results->h_length, ==, 4);
    g_assert(results->h_addr_list[0] != NULL);
    g_assert(results->h_addr_list[1] == NULL);

    inet_ntop(AF_INET, results->h_addr_list[0], buffer, INET_ADDRSTRLEN);
    g_assert_cmpstr(buffer, ==, EXPECTED_IP);
}

static void
test_gethostbyname2_inet6(void)
{
    struct hostent *results;

    results = gethostbyname2("pdf-api.local.mac", AF_INET6);

    g_assert(results == NULL);
    g_assert_cmpint(errno, ==, EAFNOSUPPORT);
    g_assert_cmpint(h_errno, ==, NO_DATA);
}

int
main (
    int argc,
    char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func("/test/test_gethostbyname",
            test_gethostbyname);
    g_test_add_func("/test/test_gethostbyname_no_suffix",
            test_gethostbyname_no_suffix);
    g_test_add_func("/test/test_gethostbyname_unknown_name",
            test_gethostbyname_unknown_name);
    g_test_add_func("/test/test_gethostbyname_exit_code",
            test_gethostbyname_exit_code);
    g_test_add_func("/test/test_gethostbyname2",
            test_gethostbyname2);
    g_test_add_func("/test/test_gethostbyname2_inet6",
            test_gethostbyname2_inet6);

    return g_test_run ();
}
