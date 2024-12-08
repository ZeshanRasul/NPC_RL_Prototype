# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

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

action_mapping = {0: "PATROL", 1: "RETREAT", 2: "ADVANCE", 3: "ATTACK"}
q_table["action_name"] = q_table["action"].map(action_mapping)


# 1. Q-Value Heatmaps
# Bin the health into levels (e.g., Low, Medium, High)
q_table["health_bin"] = pd.cut(
    q_table["health"], bins=[0, 40, 70, 100], labels=["Low", "Medium", "High"]
)

# Bin the distance_to_player into ranges
q_table["distance_bin"] = pd.cut(
    q_table["distance_to_player"], bins=[0, 15, 60, np.inf], labels=["Close", "Medium", "Far"]
)

# Create a pivot table for heatmap data (average Q-value for each action and state combination)
heatmap_data = q_table.pivot_table(
    values="q_value",
    index=["health_bin", "distance_bin"],
    columns=["action_name"],
    aggfunc="mean"
)

# Plot the heatmap
plt.figure(figsize=(14, 10))
sns.heatmap(
    heatmap_data,
    annot=True,
    fmt=".2f",
    cmap="coolwarm",
    cbar_kws={'label': 'Q-Value'}
)
plt.title("Heatmap of Q-Values by Health, Distance, Actions")
plt.xlabel("Action")
plt.ylabel("State (Health Bin, Distance Bin)")
plt.tight_layout()
plt.show()

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

# 5. State-Based Action Distributions
# Define the conditions for the states
conditions = {
    "Player Far (distance > 60)": q_table["distance_to_player"] > 60,
    "Player Medium Range (15 < distance <= 60)": (q_table["distance_to_player"] > 15) & (q_table["distance_to_player"] <= 60),
    "Player Close (distance <= 15)": q_table["distance_to_player"] <= 15,
    "Low Health (health <= 40)": q_table["health"] <= 40,
    "Suppression Fire Active": q_table["is_suppression_fire"] == 1,
    "Player Detected but Not Visible": (q_table["player_detected"] == 1) & (q_table["player_visible"] == 0),
    "Player Detected and Visible": (q_table["player_detected"] == 1) & (q_table["player_visible"] == 1),
}

# Generate bar plots for each condition
for condition_name, condition_filter in conditions.items():
    # Filter the data based on the condition
    filtered_data = q_table[condition_filter]
    
    # Group and count actions
    action_distribution = filtered_data["action_name"].value_counts().reset_index()
    action_distribution.columns = ["action_name", "count"]
    
    # Plot the distribution
    plt.figure(figsize=(8, 5))
    ax = sns.barplot(x="action_name", y="count", data=action_distribution, palette="viridis")
    
    # Annotate the exact frequency numbers on top of the bars
    for i, bar in enumerate(ax.patches):
        ax.annotate(
            f'{int(bar.get_height())}',  # Display the count as an integer
            (bar.get_x() + bar.get_width() / 2, bar.get_height()),  # Position the annotation
            ha='center', va='bottom', fontsize=10, color='black'  # Text properties
        )
    
    plt.title(f"State-Based Action Distribution: {condition_name}")
    plt.xlabel("Action")
    plt.ylabel("Frequency")
    plt.show()