#include "cost.h"
#include "cJSON.h"
#include <iostream>
#include <fstream>

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

  int _parse_cost_json_file(const char *cost_file)
  {
    size_t len = 0;
    char *str = _file2str(cost_file, len) ;
    delete[] str;
    cJSON *json = cJSON_Parse(str);
    if (!json)
    {
      cout <<  "Error before: \n"<< endl ;
      return -1;
    }
    return 0;
  }
}

CostMatrix::CostMatrix(const char *filename)
{
  _parse_cost_json_file(filename);
}

CostMatrix::~CostMatrix()
{
}
