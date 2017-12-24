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
  const int num_cities = cJSON_GetArraySize(json_array_cities);
  _cities.reserve(num_cities);
  for (int i = 0; i < num_cities; ++i)
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
      _city_indices.insert(
        pair<string, CityIdxType>(city->string, city->valueint));
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

CostMatrix::CostMapType **
CostMatrix::_create_cost_mat(cJSON *json)
{
  const size_t ndim = _cities.size();
  _cost_mat = new CostMapType *[ndim];
  for (int i = 0; i < ndim; ++i)
    _cost_mat[i] = new CostMapType[ndim];
  cJSON *json_2d = cJSON_GetObjectItem(json, "cost_matrix");
  assert(ndim == cJSON_GetArraySize(json_2d));
  vsp_debug && cout << "\nCost Matrix\n" << "[" << "\n";
  for (int i = 0; i < ndim; ++i)
  {
    vsp_debug && cout << " [" << "\n";
    cJSON *json_1d = cJSON_GetArrayItem(json_2d, i);
    assert(ndim == cJSON_GetArraySize(json_1d));
    for (int j = 0; j < ndim; ++j)
    {
      vsp_debug && cout << "  {";
      cJSON *route_cost_map = cJSON_GetArrayItem(json_1d, j);
      for (cJSON *route_cost = route_cost_map->child;
           route_cost; route_cost = route_cost->next)
      {
        const char *route_name = route_cost->string;
        double dist = cJSON_GetObjectItem(route_cost, "distance")->valuedouble;
        double expe = cJSON_GetObjectItem(route_cost, "expense")->valuedouble;
        double dura = cJSON_GetObjectItem(route_cost, "duration")->valuedouble;
        vsp_debug && cout << "\"" << route_name << "\": "
          << "{" << "\"distance\": " << dist << "," << "\"expense\": "  << expe
          << "," << "\"duration\": " << dura << "}, ";
        Cost cst(dist, expe, dura);
        pair<map<string, Cost>::iterator, bool> ret =
          _cost_mat[i][j].insert(pair<string, Cost>(route_name, cst));
        if (!ret.second)
        {
          cerr << "**Error: duplicated route name "
            << route_cost->string << " found!" << endl;
          exit(-1);
        }
      }
      vsp_debug && cout << "}," << "\n";
    }
    vsp_debug && cout << " ]," << "\n";
  }
  vsp_debug && cout << "]" << "\n";
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
  vsp_debug && cout << "\nProbability Matrix\n" << "[" << "\n";
  for (int i = 0; i < cJSON_GetArraySize(json_3d); ++i)
  {
    vsp_debug && cout << " [" << "\n";
    cJSON *json_2d = cJSON_GetArrayItem(json_3d, i);
    for (int j = 0; j < cJSON_GetArraySize(json_2d); ++j)
    {
      vsp_debug && cout << "  [";
      cJSON *json_1d = cJSON_GetArrayItem(json_2d, j);
      if (_NUM_PROB_TICKS != cJSON_GetArraySize(json_1d))
      {
        cerr << "** Error: hour ticks mismatch!" << endl;
        exit(-1);
      }
      for (int k = 0; k < cJSON_GetArraySize(json_1d); ++k)
      {
        _prob_mat[i][j][k] = cJSON_GetArrayItem(json_1d, k)->valuedouble;
        vsp_debug && cout << _prob_mat[i][j][k] << ", ";
      }
      vsp_debug && cout << "]," << "\n";
    }
    vsp_debug && cout << " ]," << "\n";
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
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
}

CostMatrix::~CostMatrix()
{
  const size_t ndim = _cities.size();
  for (int i = 0; i < ndim; ++i)
  {
    delete[] _cost_mat[i];
    delete[] _prob_mat[i];
  }
  delete[] _cost_mat;
  _cost_mat = 0;
  delete[] _prob_mat;
  _prob_mat = 0;
}

