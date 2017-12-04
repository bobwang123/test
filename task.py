
import cost
import math


def _is_reachable(avl_time, avl_loc, task, cost_prob_mat):
    """ Giving available time and location, check out if it is an reachable task according to the cost matrix. """
    assert isinstance(task, Task)
    assert isinstance(cost_prob_mat, cost.CostMatrix)
    max_empty_run_time = task.expected_start_time - avl_time
    if max_empty_run_time <= 0:
        return False
    # check out every possible route duration between current location and task
    costs = cost_prob_mat.costs(avl_loc, task.loc_from)
    for c in costs.values():
        if c.duration < max_empty_run_time:
            return True
    return False


class Route(object):
    def __init__(self, task, name=None, cost_obj=None):
        assert isinstance(task, Task)
        assert isinstance(cost_obj, cost.Cost)
        self._this_task = task  # this route belongs to task
        self._name = name
        self._cost = cost_obj
        self._expected_end_time = \
            self._this_task.expected_start_time + self._this_task.no_run_time + self._cost.duration
        self._next_steps = list()  # next reachable steps for sorting in future
        self._profit = None
        self._max_profit = None

    @property
    def name(self):
        return self._name

    @property
    def expense(self):
        if self._cost:
            return self._cost.expense
        return 0.0

    @property
    def profit(self):
        if self._profit is None:
            self._profit = self._this_task.receivable - self.expense
            if self._this_task.is_virtual:
                self._profit *= self._this_task.prob
        return self._profit

    @property
    def next_steps(self):
        return self._next_steps

    def connect(self, task, cost_prob_mat, max_wait_time=math.inf, max_empty_run_distance=math.inf):
        max_empty_run_time = task.expected_start_time - self.expected_end_time
        if max_empty_run_time <= 0:
            return False
        connected = False
        # check out every possible route duration between current location and task
        empty_run_costs = cost_prob_mat.costs(self._this_task.loc_to, task.loc_from)
        for k, c in empty_run_costs.items():
            wait_time = max_empty_run_time - c.duration
            empty_run_distance = c.distance
            if wait_time < 0 \
                    or wait_time > max_wait_time \
                    or empty_run_distance > max_empty_run_distance:
                continue
            # create an EmptyRunTask object if task can be connected via this route
            empty_run_start_time = self.expected_end_time + wait_time
            empty_run = \
                EmtpyRunTask(loc_start=self._this_task.loc_to, loc_end=task.loc_from, start_time=empty_run_start_time,
                             occur_prob=task.prob, is_virtual=task.is_virtual, wait_time=wait_time)
            empty_run_route = Route(task=empty_run, name=k, cost_obj=c)
            empty_run.add_route(empty_run_route)
            candidate_step = Step(empty_run_route=empty_run_route, order_task=task)
            self.add_next_step(candidate_step)
            connected = True
        return connected

    def update_max_profit(self):
        if self._max_profit is not None:
            return
        if not self.next_steps:
            self._max_profit = self.profit
            return
        for s in self.next_steps:
            assert isinstance(s, Step)
            if s.max_profit is None:
                s.update_max_profit()
        self.next_steps.sort(key=lambda ss: ss.max_profit, reverse=True)
        max_profit_step = self.next_steps[0]
        # max_profit_step = max(self.next_steps, key=lambda ss: ss.max_profit)
        assert isinstance(max_profit_step, Step)
        if max_profit_step.max_profit <= 0:
            self._max_profit = self.profit
        else:
            self._max_profit = self.profit + max_profit_step.max_profit

    @property
    def max_profit(self):
        return self._max_profit

    @property
    def expected_end_time(self):
        return self._expected_end_time

    def is_reachable(self, task, cost_prob_mat):
        return _is_reachable(self._expected_end_time, self._this_task.loc_to, task, cost_prob_mat)

    def add_next_step(self, step):
        assert isinstance(step, Step)
        self._next_steps.append(step)


class Task(object):
    def __init__(self, loc_start, loc_end, start_time, occur_prob, is_virtual, name):
        self._location = (loc_start, loc_end)
        self._expected_start_time = start_time  # hours since 00:00:00 on 1970/1/1
        self._occur_prob = occur_prob  # occurrence probability
        self._routes = list()
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
    def prob(self):
        return self._occur_prob

    @property
    def routes(self):
        return self._routes

    @property
    def no_run_time(self):
        """ a task may have no run time when it's waiting, loading or unloading """
        return 0

    @property
    def receivable(self):
        return 0

    @property
    def is_virtual(self):
        return self._is_virtual

    def add_route(self, route):
        assert isinstance(route, Route)
        self._routes.append(route)

    def connect(self, task, cost_prob_mat, max_wait_time=math.inf, max_empty_run_distance=math.inf):
        assert isinstance(task, Task)
        assert isinstance(cost_prob_mat, cost.CostMatrix)
        if id(self) == id(task):  # avoid connecting to itself
            return False
        connected = False
        for route in self._routes:
            connected = route.connect(task, cost_prob_mat, max_wait_time, max_empty_run_distance)
        return connected


class OrderTask(Task):
    def __init__(self, loc_start, loc_end, start_time, occur_prob=1.0, is_virtual=False, name=None,
                 receivable=0.0, load_time=0.0, unload_time=0.0):
        super(OrderTask, self).__init__(loc_start, loc_end, start_time, occur_prob, is_virtual, name)
        self._receivable = receivable  # yuan
        self._load_time = load_time  # hour
        self._unload_time = unload_time  # hour
        self._max_profit_route = None

    @property
    def load_time(self):
        return self._load_time

    @property
    def no_run_time(self):
        return self._load_time + self._unload_time

    @property
    def receivable(self):
        return self._receivable

    def max_profit_route(self):
        if self._max_profit_route is not None:
            return self._max_profit_route
        # calculate max profit recursively
        assert self.routes
        for route in self.routes:
            route.update_max_profit()
        self.routes.sort(key=lambda r: r.max_profit, reverse=True)
        self._max_profit_route = self.routes[0]
        # self._max_profit_route = max(self.routes, key=lambda r: r.max_profit)
        return self._max_profit_route


class EmtpyRunTask(Task):
    """ Waiting time is always added in front of start_time. """
    NAME_PREFIX = '!'

    def __init__(self, loc_start, loc_end, start_time, occur_prob=1.0, is_virtual=False, name=None,
                 wait_time=0.0):
        super(EmtpyRunTask, self).__init__(loc_start, loc_end, start_time, occur_prob, is_virtual, name)
        self._routes = [None]  # EmptyRunTask has and only has one Route obj
        self._wait_time = wait_time

    @property
    def no_run_time(self):
        return self._wait_time

    def add_route(self, route):
        assert isinstance(route, Route)
        # EmptyRunTask has and only has one Route obj
        self._routes[0] = route


class Step(object):
    def __init__(self, empty_run_route, order_task):
        assert isinstance(empty_run_route, Route)
        assert isinstance(order_task, OrderTask)
        self._empty_run_route = empty_run_route
        self._order_task = order_task
        self._max_profit = None  # the max sum of profit of this step and its possible consecutive steps

    @property
    def max_profit(self):
        return self._max_profit

    def update_max_profit(self):
        if self._max_profit is not None:
            return
        order_route = self._order_task.max_profit_route()
        assert isinstance(order_route, Route)
        # update max profit recursively if not done yet
        if order_route.max_profit is None:
            order_route.update_max_profit()
        self._max_profit = self._empty_run_route.profit + order_route.max_profit


class Plan(object):
    """ A plan is composed of consecutive steps """
    def __init__(self):
        self._steps = list()

    def append(self, step):
        assert isinstance(step, Step)
        self._steps.append(step)


class Vehicle(object):
    def __init__(self, name, avl_loc=None, avl_time=0, candidate_num_limit=5, plan_size_limit=math.inf):
        self._name = name
        self._avl_loc = avl_loc
        self._avl_time = avl_time  # hours
        self._reachable_orders = list()
        self._max_profit_order = None
        self._candidate_plans_sorted = Plan()
        self._candidate_num_limit = candidate_num_limit
        self._plan_size_limit = plan_size_limit

    @property
    def avl_time(self):
        return self._avl_time

    def is_reachable(self, order, cost_prob_mat):
        return _is_reachable(self._avl_time, self._avl_loc, order, cost_prob_mat)

    @property
    def reachable_orders(self):
        return self._reachable_orders

    @reachable_orders.setter
    def reachable_orders(self, orders):
        self._reachable_orders = orders

    def compute_max_profit(self):
        self._reachable_orders.sort(key=lambda order: order.max_profit_route().max_profit, reverse=True)

    def _first_step(self):
        pass

    def candidate_plans(self, cost_prob_mat):
        pass
