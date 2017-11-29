
import cost
import optparse
import scheduler

if __name__ == "__main__":
    option_parser = optparse.OptionParser()
    assert isinstance(option_parser, optparse.OptionParser)
    option_parser.add_option("--prob-threshold", action="store", dest="prob_th", type="float",
                             metavar="PROB_TH", default=0.05,
                             help="predicted orders are removed if prob < PROB_TH. [default=%default]")
    (cmd_opt, args) = option_parser.parse_args()
    cost_prob_mat = cost.CostMatrix("cost.json", "probability.http_api.json")
    schlr = scheduler.Scheduler("orders.http_api.json", "vehicles.http_api.json", cost_prob_mat, cmd_opt)
    schlr.expand_orders(cost_prob_mat)
    print("Finished scheduling.")
