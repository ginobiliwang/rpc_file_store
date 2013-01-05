#ifndef RPCHELPER_HH
#define RPCHELPER_HH

#include "scopeguard.hh"

template <class T>
class xdrhelper_scope_free
    : public scope_guard2<void, xdrproc_t, caddr_t>
{
 public:
    xdrhelper_scope_free(bool_t (*f) (XDR*, T*), T* a)
	: scope_guard2<void, xdrproc_t, caddr_t>(xdr_free, (xdrproc_t) f, (caddr_t) a) {}
};

void
rpchelper_clnt_destroy(CLIENT *clnt)
{
    clnt_destroy(clnt);
}

// Why isn't this defined in the header files?
extern "C" SVCXPRT *svcfd_create(int, u_int, u_int);

#endif
