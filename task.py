
import cost
import math


class Route(object):
    def __init__(self, task, name=None, cost_ref=None):
        self._task = task  # reference
        self._name = name
        self._cost = cost_ref
        self._next_tasks = list()  # list of Task object references

    @property
    def name(self):
        return self._name


class Task(object):
    def __init__(self, loc_start, loc_end, start_time, occur_prob, is_virtual, name):
        self._location = (loc_start, loc_end)
        self._expected_start_time = start_time  # hours since 00:00:00 on 1970/1/1
        self._occur_prob = occur_prob  # occurrence probability
        self._routes = dict()  # Route.name:Route()
        self._is_virtual = is_virtual  # true iff this is a predicted order
        self._name = name

    @property
    def name(self):
        return str(self._name)

    @property
    def loc_from(self):
        return self._location[0]

    @property
    def loc_to(self):
        return self._location[1]

    @property
    def expected_start_time(self):
        return self._expected_start_time

    @property
    def is_virtual(self):
        return self._is_virtual

    def add_route(self, route):
        assert isinstance(route, Route)
        self._routes[route.name] = route


class OrderTask(Task):
    def __init__(self, loc_start, loc_end, start_time, occur_prob=1.0, is_virtual=False, name=None,
                 receivable=0.0, load_time=0.0, unload_time=0.0):
        super(OrderTask, self).__init__(loc_start, loc_end, start_time, occur_prob, is_virtual, name)
        self._receivable = receivable  # yuan
        self._load_time = load_time  # hour
        self._unload_time = unload_time  # hour

    @property
    def load_time(self):
        return self._load_time


class EmtpyRunTask(Task):
    """ Waiting time is always added before start_time. """
    def __init__(self, loc_start, loc_end, start_time, occur_prob=1.0, is_virtual=False, name=None,
                 waiting=0.0):
        super(EmtpyRunTask, self).__init__(loc_start, loc_end, start_time, occur_prob, is_virtual, name)
        self._waiting = waiting  # waiting time


class Vehicle(object):
    def __init__(self, name, avl_loc=None, avl_time=0, num_orders_limit=math.inf):
        self._name = name
        self._avl_loc = avl_loc
        self._avl_time = avl_time  # hours
        self._orders = list()
        self._num_orders_limit = num_orders_limit

    def is_reachable(self, order, cost_prob_mat):
        assert isinstance(order, OrderTask)
        assert isinstance(cost_prob_mat, cost.CostMatrix)
        min_empty_run_duration = order.expected_start_time - order.load_time - self._avl_time
        if min_empty_run_duration <= 0:
            return False
        # check out every possible route duration between this vehicle and order
        costs = cost_prob_mat.costs(self._avl_loc, order.loc_from)
        for c in costs.values():
            if c.duration < min_empty_run_duration:
                return True
        return False
