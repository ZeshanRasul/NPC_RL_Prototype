# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

file_path = "0EnemyStateQTable.csv"
q_table = pd.read_csv(file_path)

q_table.columns = [
    "player_detected",
    "player_visible",
    "distance_to_player",
    "is_suppression_fire",
    "health",
    "action",
    "q_value"
    ]

action_mapping = {0: "ATTACK", 1: "ADVANCE", 2: "RETREAT", 3: "PATROL"}
q_table["action_name"] = q_table["action"].map(action_mapping)


# #  1.State-Based Action Distributions
# conditions = {
#     "Player Far (distance > 60)": q_table["distance_to_player"] > 60,
#     "Player Medium Range (30 < distance <= 60)": (q_table["distance_to_player"] > 30) & (q_table["distance_to_player"] <= 60),
#     "Player Close (distance <= 30)": q_table["distance_to_player"] <= 30,
#     "Low Health (health <= 40)": q_table["health"] <= 40,
#     "Suppression Fire Active": q_table["is_suppression_fire"] == 1,
#     "Player Detected but Not Visible": (q_table["player_detected"] == 1) & (q_table["player_visible"] == 0),
#     "Player Detected and Visible": (q_table["player_detected"] == 1) & (q_table["player_visible"] == 1),
# }

# for condition_name, condition_filter in conditions.items():
#     # Filter the data based on the condition
#     filtered_data = q_table[condition_filter]
    
#     # Group and count actions
#     action_distribution = filtered_data["action_name"].value_counts().reset_index()
#     action_distribution.columns = ["action_name", "count"]
    
#     # Plot the distribution
#     plt.figure(figsize=(8, 5))

#     ax = sns.barplot(x="action_name", y="count", data=action_distribution, palette="viridis")
    
#     # Annotate the exact frequency numbers on top of the bars
#     for i, bar in enumerate(ax.patches):
#         ax.annotate(
#             f'{int(bar.get_height())}',  # Display the count as an integer
#             (bar.get_x() + bar.get_width() / 2, bar.get_height()),  # Position the annotation
#             ha='center', va='bottom', fontsize=10, color='black'  # Text properties
#         )   

#     plt.title(f"State-Based Action Distribution: {condition_name}")
#     plt.xlabel("Action")
#     plt.ylabel("Frequency")
#     plt.show()

# 2. Q-Value Heatmap
heatmap_data = q_table.pivot_table(
    values="q_value",
    index=["player_detected", "player_visible"],
    columns="action_name",
    aggfunc="mean"
)

# Plot Heatmap for Q-Values
plt.figure(figsize=(10, 6))
sns.heatmap(
    heatmap_data,
    annot=True,
    fmt=".2f",
    cmap="coolwarm"
)
plt.title("Heatmap of Q-Values by State and Action")
plt.xlabel("Action")
plt.ylabel("State (Player Detected, Player Visible)")
plt.show()

# 3. Mean Q-Value Per Action
mean_q_values = q_table.groupby("action_name")["q_value"].mean().reset_index()

# Plot Mean Q-Value Per Action
plt.figure(figsize=(8, 5))
sns.barplot(x="action_name", y="q_value", data=mean_q_values, palette="viridis")
plt.title("Mean Q-Value Per Action")
plt.xlabel("Action")
plt.ylabel("Mean Q-Value")
plt.show()

# 4. Q-Value Convergence Over Time
# Add an entry index to act as a pseudo-time variable
q_table["entry_index"] = q_table.index

# Group by entry index bins (as proxy for time) and actions to compute mean Q-value
bin_size = len(q_table) // 50  # Define bin size (e.g., 50 bins)
q_table["time_bin"] = q_table["entry_index"] // bin_size

convergence_data = q_table.groupby(["time_bin", "action_name"])["q_value"].mean().reset_index()

# Pivot data for plotting
convergence_pivot = convergence_data.pivot(index="time_bin", columns="action_name", values="q_value")

# Plot Q-Value Convergence Over Time
plt.figure(figsize=(12, 6))
convergence_pivot.plot(marker="o", figsize=(12, 6))
plt.title("Q-Value Convergence Over Time")
plt.xlabel("Time (Binned Entry Count)")
plt.ylabel("Mean Q-Value")
plt.legend(title="Action")
plt.grid()
plt.show()