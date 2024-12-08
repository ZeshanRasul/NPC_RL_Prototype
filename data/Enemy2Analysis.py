# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

file_path = "2EnemyStateQTable.csv"
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
plt.title("Enemy 2: Heatmap of Q-Values by Health, Distance, Actions")
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
plt.title("Enemy 2: Heatmap of Q-Values by State and Action")
plt.xlabel("Action")
plt.ylabel("State (Player Detected, Player Visible)")
plt.show()
