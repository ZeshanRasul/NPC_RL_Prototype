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



        // TODO: End move to Enemy Update


        // TODO: End move to Renderer Draw


        // Swap buffers and poll IO events

        // TODO: Begin move to Window Main Loop



        // TODO: End move to Window Main Loop

    }
    
    return 0;
}




