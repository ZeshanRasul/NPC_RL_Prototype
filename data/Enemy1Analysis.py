import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

plt.rcParams.update({
    "font.size": 10,          
    "axes.titlesize": 11,     
    "axes.labelsize": 10,
    "xtick.labelsize": 9,
    "ytick.labelsize": 9,
    "legend.fontsize": 9,
    "figure.dpi": 300,
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
    "savefig.pad_inches": 0.02,  
})

def save_heatmap(data, out_base, title=None, xlabel="Action", ylabel="", vmin=None, vmax=None,
                 annot_size=9, cbar=True, cbar_label="Q", smallfont=False):
    """
    data: pivoted DataFrame
    out_base: path without extension
    """

    fig, ax = plt.subplots(figsize=(4.2, 3.2))

    fontsize=16
    if (smallfont):
        fontsize = 8

    hm = sns.heatmap(
        data,
        annot=True,
        fmt=".2f",
        cmap="RdBu_r",
        vmin=vmin,
        vmax=vmax,
        linewidths=0.5,
        linecolor="white",
        cbar_kws={'label': 'Q Value'},
        annot_kws={"size":fontsize}
    )

    if title is not None:
        ax.set_title(title, pad=3)
    else:
        ax.set_title("")

    ax.set_xlabel(xlabel, labelpad=2)
    ax.set_ylabel(ylabel, labelpad=2)

    ax.tick_params(axis="x", rotation=0, pad=1)
    ax.tick_params(axis="y", rotation=0, pad=1)

    if (smallfont):
        ax.tick_params(axis='x', labelsize=8, pad=4)

    fig.tight_layout(pad=0.1)

    fig.savefig(out_base + ".svg")
    fig.savefig(out_base + ".pdf")

    plt.close(fig)

# -----------------------------
# Load + preprocess
# -----------------------------
file_path = "1EnemyStateQTable.csv"
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

abs_q = max(abs(q_table["q_value"].min()), abs(q_table["q_value"].max()))

vmin = -abs_q
vmax = abs_q

q_table["action_name"] = q_table["action"].map(action_mapping)

q_table["health_bin"] = pd.cut(
    q_table["health"], bins=[0, 40, 70, 100], labels=["Low", "Medium", "High"], include_lowest=True
)

q_table["distance_bin"] = pd.cut(
    q_table["distance_to_player"], bins=[0, 15, 60, np.inf], labels=["Close", "Medium", "Far"], include_lowest=True
)

global_min = q_table["q_value"].min()
global_max = q_table["q_value"].max()

abs_max = max(abs(global_min), abs(global_max))
vmin, vmax = -1, 1

# Health x Distance
heatmap_hd = q_table.pivot_table(
    values="q_value",
    index=["health_bin", "distance_bin"],
    columns=["action_name"],
    aggfunc="mean"
)
save_heatmap(
    heatmap_hd,
    out_base="enemy1_health_distance_q_heatmap",
    title=None,  # leave None for no title (best for tiling)
    ylabel="(Health, Distance)",
    vmin=vmin,
    vmax=vmax,
    annot_size=9,
    cbar=True,
    cbar_label="Q",
    smallfont=True,
)

# Detected x Visible
heatmap_dv = q_table.pivot_table(
    values="q_value",
    index=["player_detected", "player_visible"],
    columns="action_name",
    aggfunc="mean"
)
save_heatmap(
    heatmap_dv,
    out_base="enemy1_detected_visible_q_heatmap",
    title=None,
    ylabel="(Detected, Visible)",
    vmin=vmin,
    vmax=vmax,
    annot_size=9,
    cbar=True,
    cbar_label="Q"
)

# Suppression x Health
heatmap_sh = q_table.pivot_table(
    values="q_value",
    index=["is_suppression_fire", "health_bin"],
    columns="action_name",
    aggfunc="mean"
)
save_heatmap(
    heatmap_sh,
    out_base="enemy1_suppression_health_q_heatmap",
    title=None,
    ylabel="(Suppression, Health)",
    vmin=vmin,
    vmax=vmax,
    annot_size=9,
    cbar=True,
    cbar_label="Q"
)