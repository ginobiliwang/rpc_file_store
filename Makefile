CFLAGS	:= -g
CXXFLAGS:= -g
LDFLAGS	:= -lpthread
RPCGENS	:= cf_svc.c cf_clnt.c cf_xdr.c cf.h
PROGS	:= cfc cfd

all:	$(PROGS)

.PHONY: submit.tar.gz
submit.tar.gz:
	tar -zcf submit.tar.gz Makefile *.[chx] *.{cc,hh} answers.txt

turnin:	submit.tar.gz
	uuencode submit.tar.gz submit.tar.gz | mail cs244b-staff@scs.stanford.edu,${USER}@stanford.edu

cfc:	cf_clnt.o cf_xdr.o cfc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

cfd:	cf_svc.o cf_xdr.o cfd.o cfd_ops.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(RPCGENS): cf.x
	rpcgen -M cf.x
	rpcgen -M -m cf.x > cf_svc.c

*.o:	cf.h

clean:
	rm -f *.o $(RPCGENS) $(PROGS)

