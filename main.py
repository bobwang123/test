
import cost

if __name__ == "__main__":
    cost_prob_mat = cost.CostMatrix("cost.json", "probability.http_api.json")
    print("Finished scheduling.")
