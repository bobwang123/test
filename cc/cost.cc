#include "cost.h"
#include "fileio.h"
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;

extern const int vsp_debug;

vector<string> &
CostMatrix::_create_cities(cJSON *json)
{
  cJSON *json_array_cities = cJSON_GetObjectItem(json, "cities");
  for (int i = 0; i < cJSON_GetArraySize(json_array_cities); ++i)
  {
    cJSON *json_city_name = cJSON_GetArrayItem(json_array_cities, i);
    _cities.push_back(json_city_name->valuestring);
  }
  if (vsp_debug)
    for (int i = 0; i < _cities.size(); ++i)
      cout << _cities[i] << endl;
  return _cities;
}

map<string, CostMatrix::CityIdxType> &
CostMatrix::_create_city_indices(cJSON *json)
{
  cJSON *json_city_idx = cJSON_GetObjectItem(json, "city_indices");
  assert(json_city_idx);
  bool has_dup_cities = false;
  for (cJSON *city = json_city_idx->child; city; city = city->next)
  {
    pair<map<string, CityIdxType>::iterator, bool> ret =
      _city_indices.insert(pair<string, CityIdxType>(city->string, city->valueint));
    if (!ret.second)
    {
      cerr << "**Error: duplicated city " << city->string << " found!" << endl;
      has_dup_cities = true;  // report every dup-city issue before exit()
    }
  }
  if (has_dup_cities)
    exit(EXIT_FAILURE);  // clean up things with atexit() if necessary
  if (vsp_debug)
    for (map<string, CityIdxType>::const_iterator it = _city_indices.begin();
         it != _city_indices.end(); ++it)
      cout << it->first << ',' << it->second << endl;
  return _city_indices;
}

std::map<std::string, Cost *> **
CostMatrix::_create_cost_mat(cJSON *json)
{
  return _cost_mat;
}

CostMatrix::_BaseProbArrayType **
CostMatrix::_create_prob_mat(cJSON *json)
{
  const size_t ndim = _cities.size();
  _prob_mat = new _BaseProbArrayType *[ndim];
  for (int i = 0; i < ndim; ++i)
    _prob_mat[i] = new _BaseProbArrayType[ndim];
  cJSON *json_3d = cJSON_GetObjectItem(json, "prob_matrix");
  vsp_debug && cout << "[" << "\n";
  for (int i = 0; i < cJSON_GetArraySize(json_3d); ++i)
  {
    vsp_debug && cout << "\t[" << "\n";
    cJSON *json_2d = cJSON_GetArrayItem(json_3d, i);
    for (int j = 0; j < cJSON_GetArraySize(json_2d); ++j)
    {
      vsp_debug && cout << "\t\t[";
      cJSON *json_1d = cJSON_GetArrayItem(json_2d, j);
      for (int k = 0; k < cJSON_GetArraySize(json_1d); ++k)
      {
        _prob_mat[i][j][k] = cJSON_GetArrayItem(json_1d, k)->valuedouble;
        vsp_debug && cout << _prob_mat[i][j][k] << ", ";
      }
      vsp_debug && cout << "\t\t]," << "\n";
    }
    vsp_debug && cout << "\t]," << "\n";
  }
  vsp_debug && cout << "]" << "\n";
  return _prob_mat;
}

int CostMatrix::_parse_cost_json(cJSON *json)
{
  _create_cities(json);
  _create_city_indices(json);
  _create_cost_mat(json);
  _create_prob_mat(json);
  return 0;
}

CostMatrix::CostMatrix(const char *filename)
  :_cost_mat(0), _prob_mat(0)
{
  cJSON *json = parse_json_file(filename);
  _parse_cost_json(json);
  cJSON_Delete(json);
}

CostMatrix::~CostMatrix()
{
  const size_t ndim = _cities.size();
  for (int i = 0; i < ndim; ++i)
    delete[] _prob_mat[i];
  delete[] _prob_mat;
  _prob_mat = 0;
}
