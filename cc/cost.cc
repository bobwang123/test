#include "cost.h"
#include "fileio.h"
#include <iostream>
#include <cstdlib>

using namespace std;

CostMatrix::CostMatrix(const char *filename)
{
  cJSON *json = parse_json_file(filename);
  const char *out = cJSON_Print(json);
  cJSON_Delete(json);
  cout << out << endl;
  free(const_cast<char *>(out));
}

CostMatrix::~CostMatrix()
{
}
