#ifndef CFHELPER_HH
#define CFHELPER_HH

#include <exception>
#include "cf.h"

class cf_error : public std::exception {
 public:
    cf_error(cfs_error r) : r_(r) {}
    cfs_error err() const { return r_; }

    const char *emsg() const {
	// This list of error strings should be synchronized with cf.x
	static const char *emsgs_[] = {
	    "Success",
	    "No such pathname",
	    "Pathname already exists",
	    "Not a directory",
	    "Is a directory",
	};

	if (r_ >= 0 && r_ < sizeof(emsgs_)/sizeof(emsgs_[0]))
	    return emsgs_[r_];
	return "Unknown";
    }

 private:
    cfs_error r_;
};

#endif
