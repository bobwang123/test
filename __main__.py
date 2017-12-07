
import cost
import io
import json
import optparse
import pycurl
import profile
import pstats
import scheduler
import subprocess
import timeit

CACHE_FILE = {
    "cost_file": "cost.http_api.json",
    "prob_file": "probability.http_api.json",
    "order_file": "orders.http_api.json",
    "vehicle_file": "vehicles.http_api.json",
    "output_plans_file": "output_plans.json"
}


def _download_via_api(api, cache_file):
    assert isinstance(api, str)
    if not api.startswith("http://"):
        return
    buffer = io.BytesIO()
    c = pycurl.Curl()
    c.setopt(c.URL, api)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()
    body = buffer.getvalue().decode()
    json_obj = json.loads(body, encoding="uft-8")
    # backup the data from http API to a file
    with open(cache_file, 'w') as f:
        f.write(json.dumps(json_obj, ensure_ascii=False, indent=4))


def _download_data(opt):
    _download_via_api(opt.cost_file, CACHE_FILE["cost_file"])
    _download_via_api(opt.prob_file, CACHE_FILE["prob_file"])
    _download_via_api(opt.order_file, CACHE_FILE["order_file"])
    _download_via_api(opt.vehicle_file, CACHE_FILE["vehicle_file"])


def _upload_plans(api, filename):
    assert api.startswith("http://")
    t1 = timeit.default_timer()
    upload_process = subprocess.Popen(
        ["curl", "--output", "curl.log", "--include", "--silent", "--show-error",
         "--header", "Content-Type: application/json;charset=utf8",
         "--data", "@%s" % filename,
         api],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out, err) = upload_process.communicate()
    if err:
        print("** Curl uploading error: %s" % err)
    t2 = timeit.default_timer()
    print("CPU - Upload plans: %.2f seconds" % (t2 - t1))


def main(opt):
    _download_data(opt)
    t1 = timeit.default_timer()
    cost_prob_mat = cost.CostMatrix(CACHE_FILE["cost_file"], CACHE_FILE["prob_file"])
    sch = scheduler.Scheduler(CACHE_FILE["order_file"], CACHE_FILE["vehicle_file"], cost_prob_mat, opt)
    sch.run()
    print("Finished scheduling.")
    sch.dump_plans(CACHE_FILE["output_plans_file"])
    _upload_plans(opt.plan_upload_api, CACHE_FILE["output_plans_file"])
    t2 = timeit.default_timer()
    print("CPU - Totals: %.2f seconds" % (t2 - t1))


if __name__ == "__main__":
    option_parser = optparse.OptionParser()
    assert isinstance(option_parser, optparse.OptionParser)
    option_parser.add_option("--cost", action="store", dest="cost_file",
                             default="cost.http_api.json",
                             help="set the source (local file or http API) to get the cost matrix, "
                                  "including route, distance, duration and expense information. [default=%default]")
    option_parser.add_option("--order-occur-prob", action="store", dest="prob_file",
                             default="probability.http_api.json",
                             help="set the source (local file or http API) to get the order occurrence probability "
                                  "matrix, including mean and std of normal distributions. [default=%default]")
    option_parser.add_option("--order-file", action="store", dest="order_file", type="string",
                             metavar="FILE", default="orders.http_api.json",
                             help="specifies the input order list file or an http API (JSON). [default=%default]")
    option_parser.add_option("--vehicle-file", action="store", dest="vehicle_file", type="string",
                             metavar="FILE", default="vehicles.http_api.json",
                             help="specifies the input vehicle list file or an http API (JSON). [default=%default]")
    option_parser.add_option("--plan-upload-api", action="store", dest="plan_upload_api", type="string",
                             metavar="API", default="",
                             help="specifies the API to upload the result i.e. a scheduling scheme. [default=%default]")
    option_parser.add_option("--prob-threshold", action="store", dest="prob_th", type="float",
                             metavar="PROB_TH", default=0.05,
                             help="predicted orders are removed if prob < PROB_TH. [default=%default]")
    option_parser.add_option("--max-emtpy-distance", action="store", dest="max_empty_dist", type="float",
                             metavar="MAX_EMPTY_DIST", default=500,
                             help="empty run distance between to consecutive orders must be shorter than "
                                  "MAX_EMPTY_DIST km. [default=%default]")
    option_parser.add_option("--max-wait-time", action="store", dest="max_wait_time", type="float",
                             metavar="MAX_WAIT_TIME", default=72,
                             help="waiting time between two consecutive orders must be shorter than "
                                  "MAX_WAIT_TIME hours. [default=%default]")
    option_parser.add_option("-m", "--profile", action="store_true", dest="profile",
                             default=False,
                             help="profiles this program. [default=%default]")
    option_parser.add_option("-d", "--debug", action="store_true", dest="debug",
                             default=False,
                             help="turn debug mode to dump more information. [default=%default]")

    (cmd_opt, args) = option_parser.parse_args()
    profile_outfile = "prof.out"
    if cmd_opt.profile:
        profile.run("main(cmd_opt)", profile_outfile)
        prof_stat = pstats.Stats(profile_outfile)
        prof_stat.sort_stats("cumulative").print_stats()
    else:
        main(cmd_opt)
