
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

    @property
    def name(self):
        return self._name

    @property
    def expense(self):
        if self._cost:
            return self._cost.expense
        return 0.0

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
    def no_run_time(self):
        """ a task may have no run time when it's waiting, loading or unloading """
        return 0

    @property
    def is_virtual(self):
        return self._is_virtual

    def add_route(self, route):
        assert isinstance(route, Route)
        self._routes.append(route)

    def connect(self, task, cost_prob_mat, max_wait_time=math.inf):
        assert isinstance(task, Task)
        assert isinstance(cost_prob_mat, cost.CostMatrix)
        if id(self) == id(task):  # disallow connecting to itself
            return False
        connected = False
        for route in self._routes:
            max_empty_run_time = task.expected_start_time - route.expected_end_time
            if max_empty_run_time <= 0:
                continue
            # check out every possible route duration between current location and task
            costs = cost_prob_mat.costs(self.loc_to, task.loc_from)
            for k, c in costs.items():
                wait_time = max_empty_run_time - c.duration
                if wait_time < 0 or wait_time > max_wait_time:  # unreachable or waiting too long
                    continue
                # create an EmptyRunTask object if task is reachable via this route
                empty_run_start_time = route.expected_end_time + wait_time
                empty_run_name = EmtpyRunTask.NAME_PREFIX + k
                empty_run = \
                    EmtpyRunTask(loc_start=self.loc_to, loc_end=task.loc_from, start_time=empty_run_start_time,
                                 occur_prob=task.prob, is_virtual=task.is_virtual, name=empty_run_name,
                                 wait_time=wait_time)
                empty_run_route = Route(task=empty_run, name=k, cost_obj=c)
                empty_run.add_route(empty_run_route)
                candidate_step = Step(empty_run_route=empty_run_route, order_route=route)
                route.add_next_step(candidate_step)
                connected = True
        return connected

    def profit(self):
        return [-r.expense * self.prob for r in self._routes]


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

    @property
    def no_run_time(self):
        return self._load_time + self._unload_time

    def profit(self):
        return [(self._receivable - r.expense) * self.prob for r in self._routes]


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
    def __init__(self, empty_run_route=None, order_route=None):
        self._empty_run_route = EmtpyRunTask(0, 0, 0) if not empty_run_route else empty_run_route
        self._order_route = OrderTask(0, 0, 0) if not order_route else order_route


class Plan(object):
    """ A plan is composed of consecutive steps """
    def __init__(self):
        self._steps = list()

    def append(self, step):
        assert isinstance(step, Step)
        self._steps.append(step)


class Vehicle(object):
    def __init__(self, name, avl_loc=None, avl_time=0, plan_size_limit=math.inf):
        self._name = name
        self._avl_loc = avl_loc
        self._avl_time = avl_time  # hours
        self._candidate_plans = Plan()
        self._plan_size_limit = plan_size_limit

    @property
    def avl_time(self):
        return self._avl_time

    def is_reachable(self, order, cost_prob_mat):
        return _is_reachable(self._avl_time, self._avl_loc, order, cost_prob_mat)
