# coding: utf-8

import sys
import numpy as np
from scipy.stats import norm
from MySqlComponent import MySqlComponent

# reload(sys)
# sys.setdefaultencoding('utf-8')

sqlcmd = MySqlComponent()

city_list = list()
# 目前只需要这些推荐城
cities = """
'上海','东莞','中山','佛山','广州','杭州','武汉','苏州','金华','长沙','成都','深圳','重庆'
"""
sql = """ select DISTINCT from_city FROM `opt_lines_type_relationship` oltr 
        INNER JOIN cities_matrix cm on cm.id = oltr.line_id WHERE oltr.`status` = 1 and cm.from_city in({0}) 
        """.format(cities)
#print sql

records = sqlcmd.querySql(sql)
for r in records:
    city_list.append(''.join(r))

num_city = len(city_list)
city2id = dict(zip(city_list, range(num_city)))
id2city = dict(zip(range(num_city), city_list))
risk_level = [0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
num_risk_level = len(risk_level)

# basic configurations/parameters among those cities.
city_distance = np.zeros([num_city, num_city])
#city_time = np.zeros([num_city, num_city])
city_cost = np.zeros([num_city, num_city])
avg_order_num = np.zeros([num_city, num_city])
std_order_num = np.zeros([num_city, num_city])
avg_price = np.zeros([num_city, num_city])
std_price = np.zeros([num_city, num_city])
itinerary_time = np.zeros([num_city, num_city])

# transformation matrix state, action:  T(s, a, s'):
#    current city --> take an action --> pick an order from (c,s')
trans_matrix = np.ones([num_city, num_city, num_city, num_city])

# reward need to be computed from the given equation:
#  avg_price(from_city, to_city) - cost(s, from_city) - cost(from_city-to_city)
#  1. for the time being, reward is kind of an expected value according to the status of the network: price, distance, cost and etc.
#  2. in the future, reward can include the time factor, e.g., reward/hour or reward/day.

#order_reward = np.zeros([num_city, num_city])
order_prob = np.zeros([num_city, num_city])
reward = np.zeros([num_city, num_city, num_city, num_risk_level])
#reward_per_hour = np.zeros([num_city, num_city, num_city, num_risk_level])

# In[30]:


# print(city2id['深圳'])
# print(id2city[3])
#print(str(city_list)[1:-1])
#print(num_risk_level)

# #### Read parameters from Database and dump them into file.
#

# In[6]:
city_str = str(city_list)[1:-1]
sql_open = " select cm.from_city, cm.to_city, lc.distance, num_order_avg, num_order_std ,\
             price_order_avg, price_order_std,(lc.oil_price + lc.etc_price) * lc.distance \
            from lines_cost lc, cities_matrix cm, `opt_lines_type_relationship` oltr\
            where lc.line_id = cm.id \
            and cm.id = oltr.line_id  and oltr.`status` = 1   \
            and cm.from_city in({0}) and cm.to_city in ({0})".format(cities)


# print(sql_open)

city_matrix_records = sqlcmd.querySql(sql_open)
# print(cout)

for r in city_matrix_records:
    city_distance[city2id[r[0]]][city2id[r[1]]] = r[2]
    avg_order_num[city2id[r[0]]][city2id[r[1]]] = r[3]
    std_order_num[city2id[r[0]]][city2id[r[1]]] = r[4]
    avg_price[city2id[r[0]]][city2id[r[1]]] = r[5]
    std_price[city2id[r[0]]][city2id[r[1]]] = r[6]
    city_cost[city2id[r[0]]][city2id[r[1]]] = r[7]

# #### Set up cost parameters

# In[7]:


# cost_per_kilo = 3.5
speed_with_load = 65
speed_without_load = 65
upload_time = 10
download_time = 10
fix_cost_per_hour = 51

# In[8]:
# set up the minimum order number to compute the distribution
min_order = 10

# reset order number of Chengdu to a very low level to see whether the algorithm avoid sending vehicles to Chengdu
# avg_order_num[14,:] = 0
# std_order_num[14,:] = 1
# avg_price[14,:] = 0
# std_price[14,:] = 1

# for each order, compute the accumulated probability with the given factor:
for i in range(num_city):
    for j in range(num_city):
        if i == j:
            order_prob[i][j] = 0
            continue
        avg = avg_order_num[i][j]
        std = std_order_num[i][j]
        if std != 0:
            accumulated_prob = norm.cdf(min_order, avg, std)
        else:
            if min_order <= avg:
                accumulated_prob = 0
            else:
                accumulated_prob = 1
        order_prob[i][j] = 1 - accumulated_prob

city_time = city_distance / speed_with_load
# !!!!  city cost need to be retrieved from DB.
# city_cost = city_distance * cost_per_kilo
order_reward = avg_price - city_cost

# add probability and risk preference:
# The following code should be simplified in the future
for c in range(num_city):
    for from_city in range(num_city):
        for to_city in range(num_city):
            city_time_empty = city_time[c][from_city]
            city_time_run = city_time[from_city][to_city]
            order_rwd = order_reward[from_city][to_city]
            city_cst = city_cost[c][from_city]
            prob = order_prob[from_city][to_city]
            for risk in range(num_risk_level):
                duration = city_time_empty + city_time_run + upload_time + download_time
                time_cost = duration * fix_cost_per_hour
                rwd = (order_rwd - city_cst - time_cost) * np.power(prob, 1 - risk_level[risk])
                reward[c, from_city, to_city, risk] = rwd
                #reward_per_hour[c, from_city, to_city, risk] = rwd / duration
                """
                time_cost = (city_time[c][from_city] + city_time[from_city][
                    to_city] + upload_time + download_time) * fix_cost_per_hour
                reward[c, from_city, to_city, risk] = (order_reward[from_city][to_city] - city_cost[c][
                    from_city] - time_cost) * np.power(order_prob[from_city][to_city], 1 - risk_level[risk])
                reward_per_hour[c, from_city, to_city, risk] = reward[c, from_city, to_city, risk] / (
                city_time[c][from_city] + city_time[from_city][to_city] + upload_time + download_time)
                """

# print(order_prob)
# print(city_distance)
# print(city_cost)
# print(avg_price)
# print(city_time)
# print(order_reward)
# print(reward_per_hour[0,0,:,10])


# #### Compute the cost/reward matrix.
# In this step, we need to compute the reward matrix.  To do that, we need to compute the following in advance:
# 1. order probability for each line, i.e., cumulative prob given a threshold.
# 2. compute the order reward for each line
# 3. compute the reward for each state + action

# ### Value iteration: iterate the utility/value for each state.
#
# 1. start with a random utility value
# 2. repeat until converge:
#     update utility based on their neighbors.  We use utility in the current step to solve utility in the next step.
#     U = R(s) +
# 3. utility's absolute value is not important.  Their relative comparison --> decision is.

# In[9]:


"""
Value iteration computes the utility of the current state's each action.  In other words, we compute within T steps, 
how the current state + action's reward can be.  To do this, we iterate over all steps.  In each step, the new utility is computed based on 
the utility of the previous step.  Still, the new state and action is computed to show the maximum reward of that state + action 
can be in the new step.  
This value is supposed to be converge with geometric sequence. 
"""


def value_iteration(num_city, util_amplifier, epslon, alpha, gamma, reward,
                    utility=np.random.rand(num_city, num_city, num_city),
                    trans_matrix=np.ones([num_city, num_city, num_city, num_city]), max_iter=1000):
    count = 0
    error2 = 1.0
    new_utility = np.zeros_like(utility)
    U_max = np.zeros(num_city)
    epslon2 = epslon * epslon

    while error2 > epslon2 and count < max_iter:
        for s in range(num_city):
            U_max[s] = max(max(utility[s, :, :].reshape(1, num_city * num_city)))

        for s in range(num_city):
            # build the equation:  Ut+1(s) = R(s, a) + Max(s') Ut(s, a, s') .
            for from_city in range(num_city):
                for to_city in range(num_city):
                    new_utility[s][from_city][to_city] = (1 - alpha) * utility[s][from_city][to_city] + alpha * (
                            reward[s][from_city][to_city] + gamma * U_max[to_city])
        error2 = sum(sum(np.square(new_utility - utility).reshape(1, num_city * num_city * num_city)))
        utility = new_utility.copy()
        count += 1
    print("total number of iteration: ", count)
    print("final error2: ", error2)
    return new_utility


# #### Given a city name find the next route to take

# In[10]:


def search_policy(risk_level, utility, city_name, city2id=city2id, city_distance=city_distance, max_empty_distance=200,
                  top=num_city):
    rlist = list()
    s = city2id[city_name]
    value = utility[s]
    x = np.argsort(value)

    l = [];
    for i in range(num_city):
        for j in range(num_city):
            l.append((utility[s, i, j], (i, j)))
    ranked_list = sorted(l, key=lambda l: -l[0])
    count = 0
    for i in range(len(ranked_list)):
        if count < top:
            if city_distance[s][ranked_list[i][1][0]] < max_empty_distance and city_distance[ranked_list[i][1][0]][
                ranked_list[i][1][1]] > max_empty_distance:
                if ranked_list[i][1][0] == ranked_list[i][1][1]:
                    continue
                if avg_price[ranked_list[i][1][0]][ranked_list[i][1][1]] < 1:
                    continue
                rlist.append((id2city[s], id2city[ranked_list[i][1][0]], id2city[ranked_list[i][1][1]],
                              ranked_list[i][0], risk_level))
                count = count + 1

    return rlist


# In[11]:


util_amplifier = 10
epslon = 0.01
alpha = 0.7
gamma = 0.9
max_iter = 1000
# count = 0
utility_cache_file = 'utility_cache.npy'

utility = np.zeros([num_city, num_city, num_city, num_risk_level])
# read from latest cache if existing and having the same shape
try:
    utility = np.load(utility_cache_file)
except IOError:
    utility = np.random.rand(num_city, num_city, num_city, num_risk_level)
for i in range(num_risk_level):
    print("Risk preference level:", risk_level[i])
    utility[:, :, :, i] = value_iteration(num_city, util_amplifier, epslon, alpha, gamma, reward[:, :, :, i],
                                          utility[:, :, :, i], trans_matrix, max_iter)
# make a backup (cache) for future reuse
np.save(utility_cache_file, utility)


# In[12]:
def insert_into_db(result_list):
    for r in result_list:
        sql_insert = "insert into q_value(status_city, from_city, to_city, q_value, risk_level) values ('%s', '%s', '%s', %d, %f)"  % (r[0], r[1], r[2], r[3], r[4])
        sqlcmd.executeSql(sql_insert)


sql_clean = "truncate TABLE q_value"
sqlcmd.executeSql(sql_clean)

# !!! use risk level and city as command line option
# !!! by default use all risk levels and all cities
print("---------------------------------------------------")
for s in range(num_city):
    print("-----------------")
    for r in range(num_risk_level):
        print("Risk level: ", risk_level[r])
        result_list = search_policy(risk_level[r],utility[:, :, :, r], id2city[s], city2id, city_distance,
                                    max_empty_distance=200, top=10)
        print(result_list)
        # insert_into_db(result_list)
print("---------------------------------------------------")

# In[38]:


# Insert into Q_function_value
"""
def submit_Q_function_value(utility, id2city):
    sql_clean = "truncate TABLE q_value"
    sqlcmd.executeSql(sql_clean)

    for r in range(num_risk_level):
        for i in range(num_city):
            status_city = id2city[i]
            for j in range(num_city):
                from_city = id2city[j]
                for k in range(num_city):
                    to_city = id2city[k]
                    sql_insert = "insert into q_value(status_city, from_city, to_city, q_value, risk_level) values ('%s', '%s', '%s', %d, %f)" \
                                 % (status_city, from_city, to_city, utility[i, j, k, r], risk_level[r])
                    # print(sql_insert)
                    sqlcmd.executeSql(sql_insert)

"""
# In[39]:


# print(avg_price)
# print(avg_price/city_distance)
# print(avg_price - city_distance*cost_per_kilo)
# print(avg_order_num)
# print(std_order_num)
# print(order_prob)
# print(order_reward)


# In[40]:


# print("------------")
# print(reward)
# print("------------")
# print(utility)
# print(order_prob[city2id['重庆'], city2id['金华']])
# print(avg_price[city2id['金华']])
# print(order_reward[city2id['金华']])
# print(reward[city2id['重庆']])
# print(utility[city2id['重庆']])
# print(1 - norm.cdf(min_order, 46.88, 53.46))
# print(1 - norm.cdf(min_order, 16.43, 9.9))


# In[41]:


# s = '成都'
# from_city = '成都'
# print(city_distance[city2id[from_city]],"\n", avg_price[city2id[from_city]],"\n", reward[city2id[s], city2id[from_city],:, 1],"\n", reward_per_hour[city2id[s], city2id[from_city],:, 1],"\n", utility[city2id[s], city2id[from_city],:, 10])
# print("----------------------")
#
# s = '武汉'
# from_city = '武汉'
# print(city_distance[city2id[from_city]],"\n", avg_price[city2id[from_city]],"\n",  reward[city2id[s], city2id[from_city],:, 1],"\n", reward_per_hour[city2id[s], city2id[from_city],:, 1],"\n", utility[city2id[s], city2id[from_city],:, 10])
# print("----------------------")
#
# s = '上海'
# from_city = '上海'
# print(city_distance[city2id[from_city]],"\n", avg_price[city2id[from_city]],"\n",  reward[city2id[s], city2id[from_city],:, 1],"\n", reward_per_hour[city2id[s], city2id[from_city],:, 1],"\n", utility[city2id[s], city2id[from_city],:, 10])
# print("----------------------")
# print(city_list)

