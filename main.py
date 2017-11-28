
import cost
import scheduler

if __name__ == "__main__":
    cost_prob_mat = cost.CostMatrix("cost.json", "probability.http_api.json")
    schlr = scheduler.Scheduler("orders.http_api.json", "vehicles.http_api.json", cost_prob_mat)
    print("Finished scheduling.")
