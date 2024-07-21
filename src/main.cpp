#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "App.h"

#include "Window/Window.h"
#include "Tools/Logger.h"
#include "ShaderOld.h"
#include "Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"
#include "GameObjects/Ground.h"
#include "GameObjects/Waypoint.h"
#include "GameObjects/Cell.h"
#include "Pathfinding/Grid.h"
#include "Primitives.h"



const unsigned int SCREEN_WIDTH = 960;
const unsigned int SCREEN_HEIGHT = 720;









int main()
{

    // TODO: App init
    App app(SCREEN_WIDTH, SCREEN_HEIGHT);

    app.run();





    // TODO: Begin move to Window Main Loop


    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        




        // Input
        // 
        // TODO: Check this can be removed
        //processInput(window, tabKeyCurrentlyPressed, player);

     


        // TODO: Begin move to Renderer Draw




        // TODO: End move to Renderer Draw






        // TODO: Begin move to Enemy Update

        float playerEnemyDistance = glm::distance(enemy.getPosition(), player.getPosition());

        if (playerEnemyDistance < 15.0f)
        {
            enemy.SetEnemyState(ATTACK);
        }
        else
        {
            enemy.SetEnemyState(PATROL);
        }

        switch (enemy.GetEnemyState())
        {
        case PATROL:
        {
            if (enemy.reachedDestination == false)
            {
                std::vector<glm::ivec2> path = findPath(
                    glm::ivec2(enemy.getPosition().x / CELL_SIZE, enemy.getPosition().z / CELL_SIZE),
                    glm::ivec2(currentWaypoint.x / CELL_SIZE, currentWaypoint.z / CELL_SIZE),
                    grid
                );

                if (path.empty()) {
                    std::cerr << "No path found" << std::endl;
                }
                else {
                    std::cout << "Path found: ";
                    for (const auto& step : path) {
                        std::cout << "(" << step.x << ", " << step.y << ") ";
                    }
                    std::cout << std::endl;
                }

                moveEnemy(enemy, path, deltaTime);
            }
            else
            {
                currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);
                
                std::vector<glm::ivec2> path = findPath(
                    glm::ivec2(enemy.getPosition().x / CELL_SIZE, enemy.getPosition().z / CELL_SIZE),
                    glm::ivec2(currentWaypoint.x / CELL_SIZE, currentWaypoint.z / CELL_SIZE),
                    grid
                );

                std::cout << "Finding new waypoint destination" << std::endl;

                enemy.reachedDestination = false;

                moveEnemy(enemy, path, deltaTime);
            }
            
            break;
        }
        case ATTACK:
        {
            std::vector<glm::ivec2> path = findPath(
                glm::ivec2(enemy.getPosition().x / CELL_SIZE, enemy.getPosition().z / CELL_SIZE),
                glm::ivec2(player.getPosition().x / CELL_SIZE, player.getPosition().z / CELL_SIZE),
                grid
            );

            std::cout << "Moving to Player destination" << std::endl;

            moveEnemy(enemy, path, deltaTime);

            break;
        }
        default:
            break;
        }

        // TODO: End move to Enemy Update


        // TODO: End move to Renderer Draw


        // Swap buffers and poll IO events

        // TODO: Begin move to Window Main Loop



        // TODO: End move to Window Main Loop

    }
    
    return 0;
}




