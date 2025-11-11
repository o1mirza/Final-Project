// Imports
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <optional>
#include <cmath>
#include <memory>
#include <string>
#include <map>
#include <variant>

// GLOBAL VALUES
// Constant config values
const unsigned int FPS_LOCK {60}, WIDTH {1280}, HEIGHT {720};
const float CAMERA_SPEED {2};
bool STOP_TIME {false};

// Main rendering surface 
sf::RenderWindow *window = nullptr;
// Camera object - prespective of the user
sf::View camera(sf::FloatRect({0, 0}, {WIDTH, HEIGHT}));

// 2D Vector
struct Vector2 {
    float x, y;

    Vector2(float i, float j): x(i), y(j) {}
    Vector2(int i, int j): x(static_cast<float>(i)), y(static_cast<float>(j)) {}
    Vector2(float magnitude, float x_angle, bool sad = true){
        x = magnitude * std::cos(x_angle);
        y = magnitude * std::sin(x_angle);
    }
};
struct ParameterInfo{
    std::string name;
    const int min, max;
    bool can_be_negative {false};
    bool is_required {false};
};
// User input and metadata
using UserParamaters = std::variant<Vector2, float>;
std::map<ParameterInfo, UserParamaters> projectile_parameters {};

// Static object handler - rendering
class static_object_handler{
    private:
        std::vector<std::shared_ptr<sf::Drawable>> object_list;

    public:
        static_object_handler(){}

        void add_object(sf::RectangleShape square, const sf::Color &color, Vector2 pos){
            square.setFillColor(color);
            square.setPosition({pos.x, pos.y});
            object_list.push_back(std::make_shared<sf::RectangleShape> (square));
        }

        void add_object(sf::CircleShape circle, const sf::Color &color, Vector2 pos){
            circle.setFillColor(color);
            circle.setPosition({pos.x, pos.y});
            object_list.push_back(std::make_shared<sf::CircleShape> (circle));
        }

        void draw(){
            for (const auto &object_ptr : object_list)
                window->draw(*object_ptr);
        }
};
// Initialize the object renderer
static_object_handler static_object_renderer {};

// Projectile Class

// Dotted line handler

// Handles all of the keyboard interactions
void process_keyboard(){
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)))
        camera.move({0.f, CAMERA_SPEED});

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
        camera.move({CAMERA_SPEED, 0.f});    

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S)))
        camera.move({0.f, -CAMERA_SPEED});

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D)))
        camera.move({-CAMERA_SPEED, 0.f});

    // P to pause, arrows to move in time
}

// Main window processing handler
void window_processing(){
    process_keyboard();
    static sf::Clock clock;

    // Poll and process all events
    while (const std::optional event = window->pollEvent()){
        // Send events to ImGui for GUI processing
        ImGui::SFML::ProcessEvent(*window, *event);

        // Handle closing the SFML application
        if (event->is<sf::Event::Closed>())
            window->close();
            return;
    }

    // ImGUI Drawing
    // Update and re-draw the imGUI contents
    ImGui::SFML::Update(*window, clock.restart());
    
    ImGui::Begin("Hello, world!");
    ImGui::Button("Look at this pretty button");
    ImGui::End();

    // SFML Drawing
    // Set the user view as the camera
    window->clear(sf::Color::Black);
    window->setView(camera);

    // Draw all objects
    static_object_renderer.draw();

    // Push the updates to both imGUI and SFML
    ImGui::SFML::Render(*window);
    window->display();
}

int main(){
    // Initialize the window object and limit the framerate
    auto window_obj = sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}), "Kinematics Simulator");
    window = &window_obj;
    window->setFramerateLimit(FPS_LOCK);

    // Check if the GUI mounted
    if (!ImGui::SFML::Init(*window))
        return -1;

    // Add objects to the frame
    static_object_renderer.add_object(sf::RectangleShape{sf::Vector2f(200.f, 200.f)}, sf::Color::Green, Vector2(10,10));

    // While the windo is open, run the mian processing loop
    while (window->isOpen()){
        window_processing();
    }

    // Shutdown the ImGUI safly and end execution 
    ImGui::SFML::Shutdown();
    return 0;
}
