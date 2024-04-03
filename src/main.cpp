#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float heightScale = 0.1f;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Advanced lighting
bool blinn = false;
bool blinnKeyPressed = false;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    //blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader normalMapping("resources/shaders/materialVertexShader.vs", "resources/shaders/materialFragmentShader.fs");
    // load models
    // -----------
    Model ourModel("resources/objects/floating_island(1)/scene.gltf");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model airBoyModel("resources/objects/airman/scene.gltf");
    airBoyModel.SetShaderTextureNamePrefix(("material."));

    Model flyingLightHouse("resources/objects/flying_lighthouse/scene.gltf");
    flyingLightHouse.SetShaderTextureNamePrefix("material.");

    Model baseIsland("resources/objects/base_island/scene.gltf");
    baseIsland.SetShaderTextureNamePrefix("material.");

    Model model1OnBaseIsland("resources/objects/steampunk_lighthouse/scene.gltf");
    model1OnBaseIsland.SetShaderTextureNamePrefix("material.");

    Model model2OnBaseIsland("resources/objects/da_vincis_-_flying_machine/scene.gltf");
    model2OnBaseIsland.SetShaderTextureNamePrefix("material.");

    Model treeModel("resources/objects/platano_tree/scene.gltf");
    treeModel.SetShaderTextureNamePrefix("material.");

    Model tree2Model("resources/objects/trees_low_poly/scene.gltf");
    tree2Model.SetShaderTextureNamePrefix("material.");

    Model windmillModel("resources/objects/mill-wind/scene.gltf");
    windmillModel.SetShaderTextureNamePrefix("material.");

    Model giraffeModel("resources/objects/alpaca_non-commercial/scene.gltf");
    giraffeModel.SetShaderTextureNamePrefix("material.");

    Model bigTreeModel("resources/objects/low_poly_tree_scene_free/scene.gltf");
    bigTreeModel.SetShaderTextureNamePrefix("material.");

    //load textures
    //----------------
    unsigned int daVinciDiffuse = loadTexture(FileSystem::getPath("resources/objects/da_vincis_-_flying_machine/textures/lambert14_baseColor.jpeg").c_str(), true);
    unsigned int daVinciSpecular = loadTexture(FileSystem::getPath("resources/textures/daVinciSpecularMap.png").c_str(), false);
    unsigned int daVinciNormal = loadTexture(FileSystem::getPath("resources/textures/daVinci.png").c_str(), false);
    //unsigned int daVinciDisp = loadTexture(FileSystem::getPath("").c_str(), false);
    unsigned int millwindDiffuse = loadTexture(FileSystem::getPath("resources/objects/mill-wind/textures/Texture-base_baseColor.jpeg").c_str(), true);
    unsigned int millwindSpecular = loadTexture(FileSystem::getPath("resources/textures/millwindSpecularMap.png").c_str(), false);
    unsigned int millwindNormal = loadTexture(FileSystem::getPath("resources/textures/millWind.png").c_str(), false);



    //skyBox

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.25, 0.25, 0.25);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.0f;


    // skybox vao
    unsigned int skyBoxVAO, skyBoxVBO;
    glGenVertexArrays(1, &skyBoxVAO);
    glGenBuffers(1, &skyBoxVBO);
    glBindVertexArray(skyBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/miramar_lf.jpg"),
                    FileSystem::getPath("resources/textures/miramar_rt.jpg"),
                    FileSystem::getPath("resources/textures/miramar_up.jpg"),
                    FileSystem::getPath("resources/textures/miramar_dn.jpg"),
                    FileSystem::getPath("resources/textures/miramar_ft.jpg"),
                    FileSystem::getPath("resources/textures/miramar_bk.jpg")
            };
    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(true);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Normal mapping
        normalMapping.use();
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        normalMapping.setVec3("pointLight.position", pointLight.position);
        normalMapping.setVec3("pointLight.ambient", pointLight.ambient);
        normalMapping.setVec3("pointLight.diffuse", pointLight.diffuse);
        normalMapping.setVec3("pointLight.specular", pointLight.specular);
        normalMapping.setFloat("pointLight.constant", pointLight.constant);
        normalMapping.setFloat("pointLight.linear", pointLight.linear);
        normalMapping.setFloat("pointLight.quadratic", pointLight.quadratic);
        normalMapping.setVec3("viewPos", programState->camera.Position);
        normalMapping.setFloat("material.shininess", 32.0f);
        normalMapping.setBool("blinn",true);
        normalMapping.setVec3("spotLight.position", glm::vec3(5.0f));
        normalMapping.setVec3("spotLight.direction", glm::vec3(-5.0f));
        normalMapping.setVec3("spotLight.ambient", glm::vec3(0.1f,0.1f,0.1f));
        normalMapping.setVec3("spotLight.diffuse", glm::vec3(0.3f,0.3f,0.3f));
        normalMapping.setVec3("spotLight.specular", glm::vec3(0.2f,0.2f,0.2f));
        normalMapping.setFloat("spotLight.constant", pointLight.constant);
        normalMapping.setFloat("spotLight.linear", pointLight.linear);
        normalMapping.setFloat("spotLight.quadratic", pointLight.quadratic);
        normalMapping.setFloat("spotLight.cutOff", glm::cos(glm::radians(40.0f)));
        normalMapping.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(45.0f)));
        normalMapping.setVec3("dirLight.direction", glm::vec3(0.0f,-1.0f,0.0f));
        normalMapping.setVec3("dirLight.ambient", glm::vec3(0.1f,0.1f,0.1f));
        normalMapping.setVec3("dirLight.diffuse", glm::vec3(0.5f,0.3f,0.3f));
        normalMapping.setVec3("dirLight.specular", glm::vec3(0.2f,0.2f,0.2f));
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        normalMapping.setMat4("projection", projection);
        normalMapping.setMat4("view", view);

        //daVinci on base island render
        glm::mat4 model5 = glm::mat4(1.0f);
        model5 = glm::translate(model5,
                                glm::vec3 (67,-9+cos(currentFrame)*2.0f,34.3));
        model5 = glm::rotate(model5, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model5 = glm::scale(model5, glm::vec3(0.27f));
        normalMapping.setMat4("model", model5);

        normalMapping.setInt("material.diffuse", 0);
        normalMapping.setInt("material.specular", 1);
        normalMapping.setInt("material.normal", 2);
        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, daVinciDiffuse);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, daVinciSpecular);
        // bind normal map
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, daVinciNormal);
        renderQuad();
        model2OnBaseIsland.Draw(normalMapping);

        //Windmill render
        glm::mat4 windmill = glm::mat4(1.0f);
        windmill = glm::translate(windmill,
                                  glm::vec3 (73,-10.5+cos(currentFrame)*0.1f,45));
        windmill = glm::scale(windmill, glm::vec3(0.4f));
        //windmill = glm::rotate(windmill, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        windmill = glm::rotate(windmill, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        normalMapping.setMat4("model", windmill);
        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, millwindDiffuse);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, millwindSpecular);
        // bind normal map
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, millwindNormal);
        renderQuad();

        windmillModel.Draw(normalMapping);


        // don't forget to enable shader before setting uniforms
        ourShader.use();
        pointLight.position = glm::vec3(3.0 , 3.0f, 3.0 );
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glDepthFunc(GL_LEQUAL);

        //Face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        //First small island render
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3 (68,-11+cos(currentFrame)*0.4f,20)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.1f));    // it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        //Second mini-floating island render
        glm::mat4 model0 = glm::mat4(1.0f);
        model0 = glm::translate(model0,
                                glm::vec3 (86,-15+cos(currentFrame)*0.2f,32)); // translate it down so it's at the center of the scene
        model0 = glm::scale(model0, glm::vec3(0.08f));
        ourShader.setMat4("model", model0);
        ourModel.Draw(ourShader);


        //Air boy render
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2,
                                glm::vec3 (73,-8.6+cos(currentFrame)*0.4f,24));
        model2 = glm::scale(model2, glm::vec3(0.1f));
        ourShader.setMat4("model", model2);
        airBoyModel.Draw(ourShader);

        //Base island render
        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3,
                                glm::vec3 (70,-15+cos(currentFrame)*0.1f,40));
        model3 = glm::scale(model3, glm::vec3(0.9f));
        ourShader.setMat4("model", model3);
        baseIsland.Draw(ourShader);

        //Model1 on base island render
        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4,
                                glm::vec3 (67.3,-14+cos(currentFrame)*0.1f,40.8));
        model4 = glm::rotate(model4, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model4 = glm::scale(model4, glm::vec3(0.01f));
        ourShader.setMat4("model", model4);
        model1OnBaseIsland.Draw(ourShader);



        //FLying lighthouse render
        glm::mat4 model6 = glm::mat4(1.0f);
        model6 = glm::translate(model6,
                                glm::vec3 (86.2,-13.8+cos(currentFrame)*0.2f,40)); // translate it down so it's at the center of the scene
        model6 = glm::rotate(model6, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model6 = glm::scale(model6, glm::vec3(0.03f));
        ourShader.setMat4("model", model6);
        flyingLightHouse.Draw(ourShader);

        //Tree render
        glm::mat4 tree1 = glm::mat4(1.0f);
        tree1 = glm::translate(tree1,
                               glm::vec3 (89,-13.5+cos(currentFrame)*0.2f,32));
        tree1 = glm::scale(tree1, glm::vec3(0.008f));
        tree1 = glm::rotate(tree1, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", tree1);
        treeModel.Draw(ourShader);

        //Tree2 render
        glm::mat4 tree2 = glm::mat4(1.0f);
        tree2 = glm::translate(tree2,
                               glm::vec3 (75.5,-13.2+cos(currentFrame)*0.1f,43));
        tree2 = glm::rotate(tree2, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        tree2 = glm::scale(tree2, glm::vec3(0.03f));
        ourShader.setMat4("model", tree2);
        tree2Model.Draw(ourShader);

        glm::mat4 tree21 = glm::mat4(1.0f);
        tree21 = glm::translate(tree21,
                                glm::vec3 (70.4,-13.5+cos(currentFrame)*0.1f,46));
        tree21 = glm::rotate(tree21, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        tree21 = glm::scale(tree21, glm::vec3(0.025f));
        ourShader.setMat4("model", tree21);
        tree2Model.Draw(ourShader);

        glm::mat4 tree22 = glm::mat4(1.0f);
        tree22 = glm::translate(tree22,
                                glm::vec3 (69.4,-13.2+cos(currentFrame)*0.1f,45.2));
        tree22 = glm::rotate(tree22, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        tree22 = glm::scale(tree22, glm::vec3(0.03f));
        ourShader.setMat4("model", tree22);
        tree2Model.Draw(ourShader);

        glm::mat4 tree23 = glm::mat4(1.0f);
        tree23 = glm::translate(tree23,
                                glm::vec3 (74,-13.2+cos(currentFrame)*0.1f,42));
        tree23 = glm::rotate(tree23, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        tree23 = glm::scale(tree23, glm::vec3(0.008f));
        ourShader.setMat4("model", tree23);
        treeModel.Draw(ourShader);

        glm::mat4 tree24 = glm::mat4(1.0f);
        tree24 = glm::translate(tree24,
                                glm::vec3 (69,-9.8+cos(currentFrame)*0.4f,14.2));
        tree24 = glm::rotate(tree24, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        tree24 = glm::scale(tree24, glm::vec3(0.04f));
        ourShader.setMat4("model", tree24);
        tree2Model.Draw(ourShader);

        glm::mat4 tree25 = glm::mat4(1.0f);
        tree25 = glm::translate(tree25,
                               glm::vec3 (87.5,-13.5+cos(currentFrame)*0.2f,31));
        tree25 = glm::scale(tree25, glm::vec3(0.005f));
        tree25 = glm::rotate(tree25, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", tree25);
        treeModel.Draw(ourShader);

        glm::mat4 tree13 = glm::mat4(1.0f);
        tree13 = glm::translate(tree13,
                                glm::vec3 (87.5,-13.9+cos(currentFrame)*0.2f,32));
        tree13 = glm::rotate(tree13, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        tree13 = glm::scale(tree13, glm::vec3(0.02f));
        ourShader.setMat4("model", tree13);
        tree2Model.Draw(ourShader);

        //Giraffe-alpaca render
        glm::mat4 giraffe = glm::mat4(1.0f);
        giraffe = glm::translate(giraffe,
                                glm::vec3 (69,-9.25+cos(currentFrame)*0.4f,20));
        giraffe = glm::scale(giraffe, glm::vec3(0.5f));
        ourShader.setMat4("model", giraffe);
        giraffeModel.Draw(ourShader);


        //Big tree render
        glm::mat4 bigTree = glm::mat4(1.0f);
        bigTree = glm::translate(bigTree,
                              glm::vec3 (63.7,-13.4+cos(currentFrame)*0.1f,35));
        bigTree = glm::rotate(bigTree, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        bigTree = glm::scale(bigTree, glm::vec3(0.2));
        ourShader.setMat4("model", bigTree);
        bigTreeModel.Draw(ourShader);


        // skybox cube

        skyboxShader.use();
        view[3][0] = 0;
        view[3][1] = 0;
        view[3][2] = 0;
        view[3][3] = 0;
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyBoxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    //Blinn-Phong
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed){
        blinn = !blinn;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE){
        blinnKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::Bullet();
        ImGui::Checkbox("Blinn-Phong (shortcut: B)", &blinn);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}