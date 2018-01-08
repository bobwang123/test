
import json
import optparse


def _order_per_hour_template():
    loading_and_unloading_time = 20
    loading_time = loading_and_unloading_time/2.0
    unloading_time = loading_and_unloading_time - loading_time
    return [
        {
            "orderId": "广州市->武汉市: ",
            "orderMoney": 7133,
            "fromCity": "广州市",
            "toCity": "武汉市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 17.5661538461538,
            "lineCost": 5329.374336
        },{
            "orderId": "武汉市->广州市: ",
            "orderMoney": 7000,
            "fromCity": "武汉市",
            "toCity": "广州市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 17.5661538461538,
            "lineCost": 5329.374336
        },{
            "orderId": "广州市->成都市: ",
            "orderMoney": 13100,
            "fromCity": "广州市",
            "toCity": "成都市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 28.9076923076923,
            "lineCost": 8775.6816
        },{
            "orderId": "成都市->广州市: ",
            "orderMoney": 8000,
            "fromCity": "成都市",
            "toCity": "广州市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 28.9076923076923,
            "lineCost": 8775.6816
        },{
            "orderId": "广州市->上海市: ",
            "orderMoney": 6600,
            "fromCity": "广州市",
            "toCity": "上海市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 24.5507692307692,
            "lineCost": 7225.016416
        },{
            "orderId": "上海市->广州市: ",
            "orderMoney": 10710,
            "fromCity": "上海市",
            "toCity": "广州市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 24.5507692307692,
            "lineCost": 7225.016416
        },{
            "orderId": "上海市->武汉市: ",
            "orderMoney": 7500,
            "fromCity": "上海市",
            "toCity": "武汉市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 13,
            "lineCost": 4198.494144
        },{
            "orderId": "武汉市->上海市: ",
            "orderMoney": 4500,
            "fromCity": "武汉市",
            "toCity": "上海市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 13,
            "lineCost": 4198.494144
        },{
            "orderId": "上海市->成都市: ",
            "orderMoney": 17000,
            "fromCity": "上海市",
            "toCity": "成都市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 30.4753846153846,
            "lineCost": 9919.396368
        },{
            "orderId": "成都市->上海市: ",
            "orderMoney": 7300,
            "fromCity": "成都市",
            "toCity": "上海市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 30.4753846153846,
            "lineCost": 9919.396368
        },{
            "orderId": "成都市->武汉市: ",
            "orderMoney": 10700,
            "fromCity": "成都市",
            "toCity": "武汉市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            # "duration": 19.6630769230769,
            "lineCost": 6323.425312
        },{
            "orderId": "武汉市->成都市: ",
            "orderMoney": 6000,
            "fromCity": "武汉市",
            "toCity": "成都市",
            "orderedPickupTime": None,
            "loadingTime": loading_time,
            "unLoadingTime": unloading_time,
            "isVirtual": 0,
            #"duration": 19.6630769230769,
            "lineCost": 6323.425312
        }
    ]


def _order_list_gen(num_hours):
    nh = int(round(num_hours))
    start_time = 1515340800 * 1000  # ms, 2018-1-8:00:00:00
    delta_time = 3600 * 1000  # ms
    order_list = list()
    for h in range(nh):
        ordered_id_suffix = str(h)
        ordered_pickup_time = start_time + delta_time * h
        orders = _order_per_hour_template()
        for order in orders:
            order["orderId"] += ordered_id_suffix
            order["orderedPickupTime"] = ordered_pickup_time
            order_list.append(order)
    return order_list


if __name__ == "__main__":
    option_parser = optparse.OptionParser()
    assert isinstance(option_parser, optparse.OptionParser)
    option_parser.add_option("--hours", action="store", dest="hours",
            type="int", default=240,
            help="set the number of consecutive hours for the test."
            "[default=%default]")
    (cmd_opt, args) = option_parser.parse_args()
    output_orders = {"data": _order_list_gen(cmd_opt.hours)}
    json_str = json.dumps(output_orders, ensure_ascii=False, indent=2)
    print(json_str)
