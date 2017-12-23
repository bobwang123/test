#include "fileio.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

namespace
{
  char *_file2str(const char *filename, size_t &len)
  {
    ifstream ifs (filename, ifstream::binary);
    if (!ifs)
    {
      cerr << "Cannot open file!\n" << "Error code: " << strerror(errno);
      return 0;
    }
    // get pointer to associated buffer object
    filebuf *pbuf = ifs.rdbuf();
    // get file size using buffer's members
    size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);
    pbuf->pubseekpos (0,ifs.in);
    // allocate memory to contain file data
    char *buffer = new char[size];
    // get file data
    pbuf->sgetn (buffer,size);
    ifs.close();
    len = size;
    return buffer;
  }
}

cJSON *parse_json_file(const char *filename)
{
  size_t len = 0;
  char *str = _file2str(filename, len) ;
  cJSON *json = cJSON_Parse(str);
  delete[] str;
  if (!json)
  {
    cout <<  "Error before: \n"<< endl ;
    return 0;
  }
  return json;
}
