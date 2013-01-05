#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/clnt.h>
#include "cf.h"
#include "scopeguard.hh"
#include "rpchelper.hh"
#include "cfhelper.hh"

// the following lines added by YHF
#include <sys/types.h>
#include <sys/socket.h>

class clnt_error : public std::exception {
 public:
    clnt_error(enum clnt_stat stat) : stat_(stat) {}
    enum clnt_stat stat_;
};

class bad_command_args : public std::exception {};
class command_args {
 public:
    class out_of_args : public bad_command_args {};

    command_args(int ac, char **av) : next_(0), ac_(ac), av_(av) {}

    char *pop() {
	if (next_ >= ac_)
	    throw out_of_args();
	return av_[next_++];
    }

 private:
    int next_;
    int ac_;
    char **av_;
};

static void
execute_cmd(command_args a, CLIENT *clnt)
{
    char *cmd = a.pop();
    enum clnt_stat st;

    if (!strcmp(cmd, "null")) {
	char *junk;
	st = cfs_null_1(0, 0, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);
    } else if (!strcmp(cmd, "read")) {
	char *pn = a.pop();

	cfs_read_res res;
	memset(&res, 0, sizeof(res));
	st = cfs_read_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);

	xdrhelper_scope_free<cfs_read_res> f(xdr_cfs_read_res, &res);

	if (res.errno != CFERR_NONE)
	    throw cf_error(res.errno);

	fflush(stdout);
	write(1, res.cfs_read_res_u.data.cfs_file_data_val,
		 res.cfs_read_res_u.data.cfs_file_data_len);
    } else if (!strcmp(cmd, "write")) {
	char *pn = a.pop();
	char *data = a.pop();

	char *dn = strdup(data);
	scope_guard<void, void*> cleanup(free, dn);
	int len = strlen(data);
	dn[len] = '\n';

	cfs_write_arg arg;
	arg.pn = pn;
	arg.data.cfs_file_data_val = dn;
	arg.data.cfs_file_data_len = len + 1;

	cfs_error res;
	st = cfs_write_1(&arg, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);
	if (res != CFERR_NONE)
	    throw cf_error(res);
    } else if (!strcmp(cmd, "mkdir")) {
	char *pn = a.pop();
	cfs_error res;
	st = cfs_mkdir_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);
	if (res != CFERR_NONE)
	    throw cf_error(res);
    } else if (!strcmp(cmd, "mkfile")) {
	char *pn = a.pop();
	cfs_error res;
	st = cfs_mkfile_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);
	if (res != CFERR_NONE)
	    throw cf_error(res);
    } else if (!strcmp(cmd, "rm")) {
	char *pn = a.pop();
	cfs_error res;
	st = cfs_delete_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);
	if (res != CFERR_NONE)
	    throw cf_error(res);
    } else if (!strcmp(cmd, "ls")) {
	char *pn = a.pop();

	cfs_readdir_res res;
	memset(&res, 0, sizeof(res));

	st = cfs_readdir_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);

	xdrhelper_scope_free<cfs_readdir_res> f(xdr_cfs_readdir_res, &res);

	if (res.errno != CFERR_NONE)
	    throw cf_error(res.errno);

	for (int i = 0; i < res.cfs_readdir_res_u.ents.ents_len; i++)
	    printf("%s\n", res.cfs_readdir_res_u.ents.ents_val[i]);
    } else if (!strcmp(cmd, "lsl")) {
	char *pn = a.pop();

	cfs_readdir_res res;
	memset(&res, 0, sizeof(res));

	st = cfs_readdirinfo_1(&pn, &res, clnt);
	if (st != RPC_SUCCESS)
	    throw clnt_error(st);

	xdrhelper_scope_free<cfs_readdir_res> f(xdr_cfs_readdir_res, &res);

	if (res.errno != CFERR_NONE)
	    throw cf_error(res.errno);

	for (int i = 0; i < res.cfs_readdir_res_u.ents.ents_len; i++)
	    printf("%s\n", res.cfs_readdir_res_u.ents.ents_val[i]);
    } else {
	throw bad_command_args();
    }
}

int
main(int ac, char **av)
{
    command_args a(ac, av);
    char *progname = a.pop();

    try {
	char *server = a.pop();
	char *ps = a.pop();

	int port = atoi(ps);
	if (port == 0)
	    throw bad_command_args();

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	struct hostent *he = gethostbyname(server);
	if (!he) {
	    printf("Unknown host: %s\n", server);
	    return -1;
	}
	memcpy(&sin.sin_addr, he->h_addr, sizeof(sin.sin_addr));

	int rpc_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (rpc_sock < 0) {
	    perror("socket");
	    return -1;
	}

	if (connect(rpc_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	    perror("connect");
	    return -1;
	}

	CLIENT *clnt = clnttcp_create(&sin, CFS_PROG, CFS_VERS1, &rpc_sock, 0, 0);
	if (!clnt) {
	    clnt_pcreateerror(server);
	    return -1;
	}

	scope_guard<void, CLIENT *> cleanup(rpchelper_clnt_destroy, clnt);
	execute_cmd(a, clnt);
    } catch (bad_command_args &e) {
	printf("Usage: %s server port command ...\n", progname);
	printf("Commands:\n");
	printf("  null\n");
	printf("  read pathname\n");
	printf("  write pathname data\n");
	printf("  mkdir pathname\n");
	printf("  mkfile pathname\n");
	printf("  rm pathname\n");
	printf("  ls pathname\n");
	exit(-1);
    } catch (cf_error &e) {
	fprintf(stderr, "Server reports error: %s (%d)\n", e.emsg(), e.err());
	exit(e.err());
    } catch (clnt_error &e) {
	fprintf(stderr, "Call failed: %s\n", clnt_sperrno(e.stat_));
	exit(-1);
    }

    return 0;
}
