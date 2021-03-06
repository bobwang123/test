
import json
import numpy
import random
import timeit

from scipy.stats import norm


class Cost(object):
    def __init__(self, distance=0.0, expense=0.0, duration=0.0):
        self._distance = distance  # km
        self._expense = expense  # yuan
        self._duration = duration  # hour

    @property
    def distance(self):
        return self._distance

    @property
    def expense(self):
        return self._expense

    @property
    def duration(self):
        return self._duration

    def to_dict(self):
        return {
            "distance": self._distance,
            "expense":  self._expense,
            "duration": self._duration
        }


class CostMatrix(object):
    def __init__(self, cost_file, probability_file=None):
        t1 = timeit.default_timer()
        self._cities, self._city_indices, self._cost_mat = CostMatrix._parse_cost_json_file(cost_file)
        self._num_prob_ticks = 24  # each hour each distribution per day
        self._time_zone = +8
        self._prob_mat = self._parse_prob_json_file(probability_file)
        t2 = timeit.default_timer()
        print("Wall - Init cost matrix: %.2f seconds" % (t2 - t1))

    @staticmethod
    def _parse_cost_json_file(cost_file):
        assert cost_file
        with open(cost_file) as f:
            json_obj = json.load(f, encoding="utf-8")
            cities = tuple(json_obj["data"]["cities"])
            city_indices = {name: i for i, name in enumerate(cities)}
            cost_mat = [[None] * len(cities) for _ in range(len(cities))]
            distance = tuple([tuple(row) for row in json_obj["data"]["distances"]])
            expense = tuple([tuple(row) for row in json_obj["data"]["price"]])
            duration = tuple([tuple(row) for row in json_obj["data"]["duration"]])
            city_idx_values = city_indices.values()
            for i in city_idx_values:
                for j in city_idx_values:
                    dists = distance[i][j]
                    if not dists:
                        cost_mat[i][j] = {" ": Cost()} if i == j else None
                        continue
                    assert isinstance(dists, dict)
                    costs = dict()
                    for k, dist in dists.items():
                        costs[k] = Cost(dist, expense[i][j][k], duration[i][j][k])
                    cost_mat[i][j] = costs
        cost_mat = tuple([tuple(r) for r in cost_mat])  # list(list()) to tuple(tuple())
        return cities, city_indices, cost_mat

    def _occur_to_pickup_hour_tick(self, occur_hour):
        # address order occurring time and pickup time difference
        assert 0 <= occur_hour <= self._num_prob_ticks - 1
        pickup_hour = (occur_hour + 6) % self._num_prob_ticks
        if (occur_hour > 14) and (random.random() > (1 - 0.9)):
            pickup_hour = max(8, (occur_hour + 10) % self._num_prob_ticks)
        assert 0 <= pickup_hour <= self._num_prob_ticks - 1
        return pickup_hour

    def _parse_prob_json_file(self, prob_file):
        random.seed(9973)  # use psuedo random series with fixed seed for reproducing purpose
        prob_mat = numpy.ones((len(self._city_indices), len(self._city_indices), self._num_prob_ticks))
        if not prob_file:
            return prob_mat
        with open(prob_file) as f:
            json_obj = json.load(f, encoding="utf-8")
            for norm_dist in json_obj["data"]:
                from_loc_idx = self.city_idx(norm_dist["fromCity"])
                to_loc_idx = self.city_idx(norm_dist["toCity"])
                if from_loc_idx is None or to_loc_idx is None:
                    continue
                pickup_hour = self._occur_to_pickup_hour_tick(int(norm_dist["timeHour"]))
                mean = norm_dist["avgCount"]
                std = norm_dist["std"]
                x = 1  # number of orders appearing at this time. TODO: could be a config option?
                # pcx is the probability that an order does NOT exist
                pcx = norm.cdf(x, mean, std) if mean > 0.0 and std > 0.0 else 1.0
                # print("Pc(x)=%f, x=%f, mean=%f, std=%f" % (pcx, x, mean, std))
                assert 0.0 <= pcx <= 1.0
                prob_mat[from_loc_idx][to_loc_idx][pickup_hour] *= pcx
        # Notice: prob = 1 - pc
        return tuple([tuple([tuple([1 - pc for pc in to_data]) for to_data in from_data]) for from_data in prob_mat])

    def city_idx(self, name):
        return self._city_indices[name] if name in self._city_indices else None

    def city_name(self, idx):
        return self._cities[idx] if 0 <= idx < len(self._cities) else None

    def prob(self, start_loc, end_loc, time_in_hour):
        assert time_in_hour >= 0
        return self._prob_mat[start_loc][end_loc][int(time_in_hour + self._time_zone) % self._num_prob_ticks]

    def costs(self, start_loc, end_loc):
        return self._cost_mat[start_loc][end_loc]

    def to_json(self, filename):
        cost_mat_dict = [[{r: c.to_dict() for r, c in d2.items()} for d2 in d1] for d1 in self._cost_mat]
        json_dict = {
            "cities": self._cities,
            "city_indices": self._city_indices,
            "cost_matrix": cost_mat_dict,
            "prob_matrix": self._prob_mat
        }
        with open(filename, "w") as f:
            json.dump(json_dict, f, ensure_ascii=False)


if __name__ == "__main__":
    import sys
    cm = None
    if len(sys.argv) == 1:
        print("** Error: No cost file and probability file is given.")
        exit(-1)
    cf = sys.argv[1]
    pf = sys.argv[2] if len(sys.argv) > 2 else None
    cm = CostMatrix(cf, pf)
    cm.to_json("cost_prob.cc.json")
