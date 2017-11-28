
import cost
import json
import task


def _ms2hours(ms):
    # milliseconds to hours since 1970-1-1 00:00:00
    assert ms >= 0
    return ms / 3600000.0


class Scheduler(object):
    def __init__(self, order_file, vehicle_file, cost_prob, opt):
        self._vehicles = Scheduler._init_vehicles_from_json(vehicle_file, cost_prob)
        self._orders = Scheduler._init_order_tasks_from_json(order_file, cost_prob, self._vehicles, opt=opt)

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
        reachable_orders = list()
        orig_len = reduced_len
        for order in large_prob_orders:
            for vehicle in vehicles:
                if vehicle.is_reachable(order, cost_prob):
                    reachable_orders.append(order)
                else:
                    if not order.is_virtual:
                        print("Warning: ignore an unreachable real order %s!" % order.name)
                    continue
        reduced_len = len(reachable_orders)
        print("Ignore %d unreachable orders in total %d large probability orders." % (orig_len - reduced_len, orig_len))
        print("%d reachable large probability orders to be scheduled." % reduced_len)
        return reachable_orders

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
