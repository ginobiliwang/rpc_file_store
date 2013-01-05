#ifndef CFD_FS_HH
#define CFD_FS_HH

#include <exception>
#include <string>
#include <map>
#include <vector>

#include "cfhelper.hh"

class cfd_fs_no_such_name : public cf_error {
 public:
    cfd_fs_no_such_name() : cf_error(CFERR_NO_SUCH_NAME) {}
};

class cfd_fs_name_exists : public cf_error {
 public:
    cfd_fs_name_exists() : cf_error(CFERR_NAME_EXISTS) {}
};

class cfd_fs_node {
 public:
    virtual bool is_a_directory() = 0;
};

class cfd_fs_file : public cfd_fs_node {
 public:
    cfd_fs_file() : len_(0), data_(new char[0]) {}
    ~cfd_fs_file() { delete data_; }

    virtual bool is_a_directory() { return 0; }
    int get_length() { return len_; }

    void read(char *buf, int len) {
	if (len != len_)
	    throw std::exception();
	memcpy(buf, data_, len_);
    }

    void write(char *buf, int len) {
	char *od = data_;
	data_ = new char[len];
	len_ = len;
	delete od;
	memcpy(data_, buf, len);
    }

 private:
    int len_;
    char *data_;
};

class cfd_fs_dir : public cfd_fs_node {
 public:
    cfd_fs_dir() {}
    ~cfd_fs_dir() {
	for (std::map<std::string, cfd_fs_node*>::iterator i = m_.begin();
	     i != m_.end(); i++)
	{
	    delete i->second;
	}
    }
    virtual bool is_a_directory() { return 1; }

    void mkdir(std::string name) {
	if (m_.find(name) != m_.end())
	    throw cfd_fs_name_exists();
	m_[name] = new cfd_fs_dir();
    }

    void mkfile(std::string name) {
	if (m_.find(name) != m_.end())
	    throw cfd_fs_name_exists();
	m_[name] = new cfd_fs_file();
    }

    void remove(std::string name) {
	std::map<std::string, cfd_fs_node*>::iterator i = m_.find(name);
	if (i == m_.end())
	    throw cfd_fs_no_such_name();
	delete i->second;
	m_.erase(i);
    }

    cfd_fs_node *lookup(std::string name) {
	std::map<std::string, cfd_fs_node*>::iterator i = m_.find(name);
	if (i == m_.end())
	    throw cfd_fs_no_such_name();
	return i->second;
    }

    std::vector<std::string> readdir() {
	std::vector<std::string> v;
	for (std::map<std::string, cfd_fs_node*>::iterator i = m_.begin();
	     i != m_.end(); i++)
	    v.push_back(i->first);
	return v;
    }

    std::vector<std::string> readdirinfo() {
	std::vector<std::string> v;
	for (std::map<std::string, cfd_fs_node*>::iterator i = m_.begin();
	     i != m_.end(); i++)
	{
	    	cfd_fs_node *tmpNode = i->second;
		std::string tmpstr;
		if(tmpNode->is_a_directory())
		{		
			//parse the string
			tmpstr = i->first;
			tmpstr.append("          DIR          ");
		}
		else
		{
			//get the file size
			tmpstr = i->first;
			cfd_fs_file *f = (cfd_fs_file *)(i->second);
			int len = f->get_length();
			tmpstr.append("          FILE         ");
			char tmpSl[16] = {0};
			sprintf(tmpSl, "%d", len);
			tmpstr.append(tmpSl);
		}
		//v.push_back(i->first);
		v.push_back(tmpstr);
	}
	return v;
    }

 private:
    std::map<std::string, cfd_fs_node*> m_;
};

#endif
