#include "cost.h"
#include "fileio.h"
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;

vector<string> &
CostMatrix::_create_cities(cJSON *json)
{
  cJSON *json_array_cities = cJSON_GetObjectItem(json, "cities");
  for (int i = 0; i < cJSON_GetArraySize(json_array_cities); ++i)
  {
    cJSON *json_city_name = cJSON_GetArrayItem(json_array_cities, i);
    _cities.push_back(json_city_name->valuestring);
  }
#ifdef DEBUG
  for (int i = 0; i < _cities.size(); ++i)
    cout << _cities[i] << endl;
#endif  // DEBUG
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
#ifdef DEBUG
  for (map<string, CityIdxType>::const_iterator it = _city_indices.begin();
       it != _city_indices.end(); ++it)
    cout << it->first << ',' << it->second << endl;
#endif  // DEBUG
  return _city_indices;
}

const Cost **
CostMatrix::_create_cost_mat(cJSON *json)
{
  return 0;
}

const Cost **
CostMatrix::_create_prob_mat(cJSON *json)
{
  return 0;
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
}
