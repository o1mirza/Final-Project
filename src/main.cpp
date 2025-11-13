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
#include <algorithm>

// GLOBAL VALUES
// Constant config values
const unsigned int FPS_LOCK {60}, WIDTH {1280}, HEIGHT {720};
const float CAMERA_SPEED {2};
bool STOP_TIME {false};

// Main rendering surface 
sf::RenderWindow *window = nullptr;
// Camera object - perspective of the user
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

// Static object handler
class static_object_manager{
    protected:
        using shape_ptr = std::shared_ptr<sf::Shape>;
        std::vector<shape_ptr> object_list;

    public:
        static_object_manager(){}

        shape_ptr add_object(sf::RectangleShape square, const sf::Color &color, const Vector2 &pos){
            square.setFillColor(color);
            square.setPosition({pos.x, pos.y});
            shape_ptr shape = std::make_shared<sf::RectangleShape>(square);
            object_list.push_back(shape);
            return shape;
        }

        shape_ptr add_object(sf::CircleShape circle, const sf::Color &color, const Vector2 &pos){
            circle.setFillColor(color);
            circle.setPosition({pos.x, pos.y});
            shape_ptr shape = std::make_shared<sf::CircleShape>(circle);
            object_list.push_back(shape);
            return shape;
        }

        void draw(){
            for (const auto &object_ptr : object_list)
                window->draw(*object_ptr);
        }

        void delete_object(shape_ptr &shape){
            // Reset the pointer
            shape.reset();
            // Remove the pointer from the list
            auto new_vct_end = std::remove(object_list.begin(), object_list.end(), shape);
            object_list.erase(new_vct_end, object_list.end());
        }
};
// Initialize the static object renderer
static_object_manager static_object_renderer {};

// Dynamic object handler
class dynamic_object_manager: public static_object_manager{
    public:
        dynamic_object_manager(){}

        void move(const std::unique_ptr<sf::Shape> obj, const Vector2 &d_pos){
            obj->move({d_pos.x, d_pos.y});
        }

};
// Initialize the dynamic object handler
dynamic_object_manager dynamic_object_handler {};

// Projectile Class
// Config
class projectile_manager {
    private:
        std::unique_ptr<dynamic_object_manager> object_ptr;
    
    public:
        projectile_manager(){
            auto object_ptr = dynamic_object_handler.add_object(sf::CircleShape{30.f}, sf::Color::Red, Vector2(10,10));
        }

        void move(const Vector2 &pos){

        }

};

// Dotted line handler
class dotted_line_manager {};

// Handles all of the keyboard interactions
void process_keyboard(){
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)))
        camera.move({0.f, CAMERA_SPEED});

    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
        camera.move({CAMERA_SPEED, 0.f});    

    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S)))
        camera.move({0.f, -CAMERA_SPEED});

    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D)))
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
    dynamic_object_handler.draw();

    // Push the updates to both imGUI and SFML
    ImGui::SFML::Render(*window);
    window->display();
}

int main(){
    // Initialize the window object and limit the framerate
    auto window_obj = sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}), "Kinematics Simulator");
    window = &window_obj;
    window->setFramerateLimit(FPS_LOCK);

    // Center the screen to the display
    auto desktop = sf::VideoMode::getDesktopMode();
    window_obj.setPosition({
        (static_cast<int>(desktop.size.x) / 2) - (static_cast<int>(window_obj.getSize().x) / 2),
        (static_cast<int>(desktop.size.y) / 2) - (static_cast<int>(window_obj.getSize().y) / 2)
    });

    // Check if the GUI is mounted
    if (!ImGui::SFML::Init(*window))
        return -1;

    // Add objects to the frame
    //static_object_renderer.add_object(sf::RectangleShape({200.f, 200.f}), sf::Color::Green, Vector2(10,10));
    dynamic_object_handler.add_object(sf::CircleShape{30.f}, sf::Color::Red, Vector2(10,10));

    // While the window is open, run the main processing loop
    while (window->isOpen()){
        window_processing();
    }

    // Shutdown the ImGUI safely and end execution 
    ImGui::SFML::Shutdown();
    return 0;
}
