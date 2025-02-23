# Reinforcement Learning for Enemy NPCs in a Third Person Shooter Video Game

## Overview
This prototype was developed as the artefact component of my MSc thesis, *Enhancing Enemy AI Behaviours with Q-Learning: A Reinforcement Learning Approach for Non-Playable Characters in a Third Person Shooter*.

This project utilised **Q-Learning** techniques to create **adaptive and realistic enemy AI** in the context of a third person shooter video game developed in **C++** with **OpenGL** 4.6. As well as exploring **machine learning in game AI**, the artefact development involved **low-level C++ programming**, **real-time rendering techniques** and **advanced mathematical foundations** required for both AI research and video game engine development. 

## Core Features

 - **Q-Learning for Enemy AI decisions:** Each enemy makes **data-driven decisions** by learning optimal behaviours during training considering the complex state of the game.
 - **Event-Driven Behaviour Trees:** Traditional video game AI implementation used to compare performance and intelligence of Q-Learning with well-established industry AI techniques.
 - **A\* Pathfinding:** Custom grid-based pathfinding implementation for enemy movement, crowd navigation, and obstacle avoidance.
 - **Physics-Based Combat:** **Ray-AABB intersection tests** with raycast shooting combat.
 - **Mathematical Techniques:** **Linear algebra** including the use of **vectors and matrices** for transformations, combat, and rendering techniques. **Probability** for efficient **epsilon-greedy policy** during enemy agent training.
 - **Dual-Quaternion Skeletal Animation:** Advanced use of dual-quaternions to perform efficient animation skinning on the GPU.
 - **Physically-Based Realistic Rendering:** Advanced lighting techniques using GPU shader programs implementing the **Cook-Torrance Bidirectional Reflectance Distribution Function (BRDF)** and **Percentage Closer Filtering (PCF) shadow maps** for dynamic shadows.

## Q-Learning Implementation

**Q-Learning** is a model-free **reinforcement learning** technique used for agent decision-making driven by reward and punishment allocation. It is widely used in creating agents that can intelligently play games but is under-explored in the development of agents within a game world. The algorithm operates through:

 - **State Representation:** An encoded structure containing game state properties relevant to intelligent enemy decision-making. Includes information such as **distance to player, player visibility and individual health levels of enemies**.
 - **Action Selection:** Probability-driven action selection during training using an **epsilon-greedy policy** with a **decaying exploration rate over time**.
 - **Reward Function:** Used to encourage **optimal behaviours**, rewarding intelligent behaviours such as taking cover when health is low, chasing the player when the player has been detected but is taking cover, and retreating to safety when health is low.
 
### Bellman Equation for Optimisation:

The **Bellman Equation** is used to **update Q-values for decision-making** in a given game state:

$$ Q(s, a) \leftarrow (1 - \alpha) Q(s, a) + \alpha \left[ r + \gamma \max_{a'} Q(s', a') \right] $$

Where:
- $Q(s, a)$ is the **Q-value** for state $s$ and action $a$.
- $\alpha$ **(learning rate)** controls how much new information overrides old information.
- $r$ is the **immediate reward received** after taking action $a$.
- $\gamma$ **(discount factor)** determines the importance of future rewards.
- $\max_{a'} Q(s', a')$ is the **highest estimated Q-value for the next state** $s'$ over all possible actions $a'$.
