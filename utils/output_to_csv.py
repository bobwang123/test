
import json
import optparse

if __name__ == "__main__":
    option_parser = optparse.OptionParser()
    assert isinstance(option_parser, optparse.OptionParser)
    option_parser.add_option("--file", action="store", dest="file", type="string", help="set the file to be converted")

    (cmd_opt, args) = option_parser.parse_args()
    with open(cmd_opt.file) as f:
        obj = json.load(f, encoding="utf-8")
    plan0 = obj["data"][0]["plans"][0]
    route_template = {
            "orderId":	"natureorder_5603_5542_15",
            "orderMoney":	5600,
            "fromCity":	"成都",
            "toCity":	"武汉",
            "orderedPickupTime":	1524556800000,
            "loadingTime":	12.120000,
            "unLoadingTime":	0,
            "isVirtual":	0,
            "routeKey":	"5542",
            "distance":	1189,
            "duration":	19.820000,
            "expense":	6499.070000,
            "probability":	1,
            "waitingTime":	0,
            "lineCost":	6499.070000
            }
    route_keys = [k for k in route_template]
    print(",".join(route_keys))
    for route in plan0["routes"]:
        route_items = list()
        for route_key in route_keys:
            route_items.append(str(route[route_key]))
        print(",".join(route_items))
