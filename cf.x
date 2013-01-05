/*
 * Protocol definition for the class file system (CFS) protocol.
 */

typedef string cfs_pathname<256>;
typedef opaque cfs_file_data<4096>;

/* This list of error strings should be synchronized with cfhelper.hh */
enum cfs_error {
    CFERR_NONE = 0,
    CFERR_NO_SUCH_NAME,
    CFERR_NAME_EXISTS,
    CFERR_NOT_A_DIRECTORY,
    CFERR_IS_A_DIRECTORY
};

union cfs_read_res switch (cfs_error errno) {
case CFERR_NONE:
    cfs_file_data data;
default:
    void;
};

struct cfs_write_arg {
    cfs_pathname pn;
    cfs_file_data data;
};

union cfs_readdir_res switch (cfs_error errno) {
case CFERR_NONE:
    cfs_pathname ents<>;
default:
    void;
};

program CFS_PROG {
    version CFS_VERS1 {
	void
	CFS_NULL(void) = 0;

	cfs_read_res
	CFS_READ(cfs_pathname) = 1;

	cfs_error
	CFS_WRITE(cfs_write_arg) = 2;

	cfs_error
	CFS_MKDIR(cfs_pathname) = 3;

	cfs_error
	CFS_MKFILE(cfs_pathname) = 4;

	cfs_error
	CFS_DELETE(cfs_pathname) = 5;

	cfs_readdir_res
	CFS_READDIR(cfs_pathname) = 6;
	
	cfs_readdir_res
	CFS_READDIRINFO(cfs_pathname) = 7;
    } = 1;
} = 422222;

