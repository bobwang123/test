
import cost
import optparse
import profile
import pstats
import scheduler
import timeit


def main(opt):
    cost_prob_mat = cost.CostMatrix("cost.json", "probability.http_api.json")
    sch = scheduler.Scheduler("orders.indent.json", "vehicles.http_api.json", cost_prob_mat, opt)
    sch.analyze_orders()
    print("Finished scheduling.")
    t2 = timeit.default_timer()
    print("CPU - Totals: %.2f seconds" % (t2 - t1))


if __name__ == "__main__":
    t1 = timeit.default_timer()
    option_parser = optparse.OptionParser()
    assert isinstance(option_parser, optparse.OptionParser)
    option_parser.add_option("--max-wait-time", action="store", dest="max_wait_time", type="float",
                             metavar="MAX_WAIT_TIME", default=72,
                             help="waiting time between two consecutive orders cannot be longer than MAX_WAIT_TIME. "
                                  "[default=%default]")
    option_parser.add_option("--prob-threshold", action="store", dest="prob_th", type="float",
                             metavar="PROB_TH", default=0.05,
                             help="predicted orders are removed if prob < PROB_TH. [default=%default]")
    option_parser.add_option("-m", "--profile", action="store_true", dest="profile",
                             default=False,
                             help="profiles this program. [default=%default]")
    (cmd_opt, args) = option_parser.parse_args()
    profile_outfile = "prof.out"
    if cmd_opt.profile:
        profile.run("main(cmd_opt)", profile_outfile)
        prof_stat = pstats.Stats(profile_outfile)
        prof_stat.sort_stats("cumulative").print_stats()
    else:
        main(cmd_opt)
