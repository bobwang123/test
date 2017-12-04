
import cost
import json
import task
import timeit


def _ms2hours(ms):
    # milliseconds to hours since 1970-1-1 00:00:00
    assert ms >= 0
    return ms / 3600000.0


class Scheduler(object):
    def __init__(self, order_file, vehicle_file, cost_prob, opt):
        assert isinstance(cost_prob, cost.CostMatrix)
        self._opt = opt
        t1 = timeit.default_timer()
        vehicles = Scheduler._init_vehicles_from_json(vehicle_file, cost_prob)
        vehicles.sort(key=lambda x: x.avl_time)
        self._sorted_vehicles = vehicles
        t2 = timeit.default_timer()
        print("CPU - Init sorted vehicles: %.2f seconds" % (t2 - t1))
        t1 = timeit.default_timer()
        orders = Scheduler._init_order_tasks_from_json(order_file, cost_prob, self._sorted_vehicles, opt=self._opt)
        orders.sort(key=lambda x: x.expected_start_time)
        self._sorted_orders = orders
        self._cost_prob = cost_prob
        t2 = timeit.default_timer()
        print("CPU - Init sorted orders: %.2f seconds" % (t2 - t1))

    @staticmethod
    def _init_order_tasks_from_json(filename, cost_prob, vehicles=None, opt=None):
        assert filename
        assert isinstance(cost_prob, cost.CostMatrix)
        with open(filename) as f:
            json_obj = json.load(f)
            large_prob_orders = list()
            for record in json_obj["data"]:
                from_loc, to_loc = cost_prob.city_idx(record["fromCity"]), cost_prob.city_idx(record["toCity"])
                expected_start_time = _ms2hours(record["orderedPickupTime"])
                prob = cost_prob.prob(from_loc, to_loc, expected_start_time)
                is_virtual = False if "isVirtual" not in record else record["isVirtual"]
                if is_virtual and prob < opt.prob_th:  # ignore small probability predicted orders
                    continue
                name = record["orderId"]
                receivable = record["orderMoney"]
                load_time, unload_time = record["loadingTime"], record["unLoadingTime"]
                load_time = load_time if load_time else 5.0  # default loading time
                unload_time = unload_time if unload_time else 5.0  # default unloading time
                large_prob_orders.append(task.OrderTask(from_loc, to_loc, expected_start_time, occur_prob=prob,
                                                        is_virtual=is_virtual, name=name, receivable=receivable,
                                                        load_time=load_time, unload_time=unload_time))
        orig_len, reduced_len = len(json_obj["data"]), len(large_prob_orders)
        assert orig_len >= reduced_len
        print("Ignore %d virtual (predicted) orders in total %d orders due to probability < %.1f%%."
              % (orig_len - reduced_len, orig_len, opt.prob_th * 100))
        if not vehicles:
            return large_prob_orders
        # ignore unreachable orders
        all_reachable_orders = list()
        orig_len = reduced_len
        for vehicle in vehicles:
            reachable_orders = list()
            for order in large_prob_orders:
                if vehicle.is_reachable(order, cost_prob):
                    reachable_orders.append(order)
            vehicle.reachable_orders = reachable_orders
            all_reachable_orders.extend(reachable_orders)
        reduced_len = len(all_reachable_orders)
        print("Ignore %d unreachable orders in total %d large probability orders." % (orig_len - reduced_len, orig_len))
        print("%d reachable large probability orders to be scheduled." % reduced_len)
        return all_reachable_orders

    @staticmethod
    def _init_vehicles_from_json(filename, cost_prob):
        assert filename
        assert isinstance(cost_prob, cost.CostMatrix)
        with open(filename) as f:
            json_obj = json.load(f)
            vehicles = list()
            for record in json_obj["data"]:
                name = record["vehicleNo"]
                avl_loc = cost_prob.city_idx(record["availableCity"])
                avl_time = _ms2hours(record["earliestAvailableTime"])
                vehicles.append(task.Vehicle(name, avl_loc, avl_time))
        return vehicles

    def _build_order_cost(self):
        for order in self._sorted_orders:
            costs = self._cost_prob.costs(order.loc_from, order.loc_to)
            for k, c in costs.items():
                route = task.Route(task=order, name=k, cost_obj=c)
                order.add_route(route)

    def _build_order_dag(self):
        sorted_orders = self._sorted_orders
        num_edges = 0
        # TODO: outer loop can be parallel
        for i, order in enumerate(sorted_orders):
            for next_i in range(i+1, len(sorted_orders)):
                next_candidate = sorted_orders[next_i]
                if order.connect(next_candidate, self._cost_prob, self._opt.max_wait_time, self._opt.max_empty_dist):
                    num_edges += 1
        print("Create %d edges for order DAG." % num_edges)

    def _analyze_orders(self):
        t1 = timeit.default_timer()
        self._build_order_cost()
        t2 = timeit.default_timer()
        print("CPU - Build order cost sorted vehicles: %.2f seconds" % (t2 - t1))
        t1 = timeit.default_timer()
        self._build_order_dag()
        t2 = timeit.default_timer()
        print("CPU - Build order DAG sorted vehicles: %.2f seconds" % (t2 - t1))

    def run(self):
        self._analyze_orders()
        t1 = timeit.default_timer()
        for vehicle in self._sorted_vehicles:
            vehicle.compute_max_profit()
        t2 = timeit.default_timer()
        print("CPU - Compute max profit for all vehicles: %.2f seconds" % (t2 - t1))
