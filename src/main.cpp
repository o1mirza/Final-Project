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

    Vector2(): x(0.f), y(0.f) {}
    Vector2(float i, float j): x(i), y(j) {}
    Vector2(int i, int j): x(static_cast<float>(i)), y(static_cast<float>(j)) {}
    Vector2(float magnitude, float x_angle, bool sad = true){
        x = magnitude * std::cos(x_angle);
        y = magnitude * std::sin(x_angle);
    }
};
struct ParameterInfo{
    float value {};
    const int min {}, max {};
    bool is_required {};
};
// User input and metadata
std::map<std::string, ParameterInfo> projectile_parameters {
    {"Initial Velocity", ParameterInfo{0.f, 0, 1000, true}},
    {"Final Velocity", ParameterInfo{0.f, 0, 1000, true}},
    {"Initial Speed", ParameterInfo{0.f, 0, 1000, true}},
    {"Final Speed", ParameterInfo{0.f, 0, 1000, true}},
    {"Initial Height", ParameterInfo{0.f, 0, 1000, true}},
    {"coeff Friction", ParameterInfo{0.f, 0, 1, true}},
    {"X Acceleration", ParameterInfo{0.f, 0, 1000, true}},
    {"Y Acceleration", ParameterInfo{0.f, 0, 1000, true}},
    {"Initial Force", ParameterInfo{0.f, 0, 1000, true}},
    {"Final Force", ParameterInfo{0.f, 0, 1000, true}},
    {"WRT x Angle", ParameterInfo{0.f, 0, 360, true}},
    {"Time Elapsed", ParameterInfo{0.f, 0, 1000, true}},
    {"Mass", ParameterInfo{0.f, 0, 1000, true}},
    {"Distance", ParameterInfo{0.f, 0, 1000, true}},
};

// Validate input

// Static object handler
using shape_ptr = std::shared_ptr<sf::Shape>;
class static_object_manager{
    protected:
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

        void move(const shape_ptr obj, const Vector2 &d_pos){
            obj->move({d_pos.x, d_pos.y});
        }

};
// Initialize the dynamic object handler
dynamic_object_manager dynamic_object_handler {};

// Projectile Class
// Config
class projectile_manager {
    private:
        std::unique_ptr<shape_ptr> object_ptr;
    
    public:
        projectile_manager(){
            shape_ptr object_ptr = dynamic_object_handler.add_object(sf::CircleShape{30.f}, sf::Color::Red, Vector2(10,10));
        }

        void move(const Vector2 &pos){

        }
};

// Dotted line handler
class dotted_line_manager {
    private:
        std::vector<shape_ptr> line_list {};

    public:
        dotted_line_manager(){}

        void reformat_line(const std::vector<Vector2> &pos_list){
            for (shape_ptr ptr : line_list){
                static_object_renderer.delete_object(ptr);
            }

            for (Vector2 pos : pos_list){
                static_object_renderer.add_object(sf::CircleShape{4.f}, sf::Color::White, pos);
            }
        }
};

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
    
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_P)))
        for (const auto& pair : projectile_parameters) {
            const std::string& key = pair.first;
            const ParameterInfo value = pair.second;
            std::cout <<  key << " : " << value.value << std::endl;
    }
}

void render_gui(){
    // --- Input Tab ---
    // Set initial position of the window
    ImGui::SetNextWindowPos(ImVec2(10.f,10.f), ImGuiCond_Once);
    ImGui::Begin("Input Tab");

    // --- Layout Settings ---
    ImGui::PushItemWidth(120.0f);
    // --- Input Fields ---
    ImGui::Text("Initial Velocity (m/s):");    ImGui::SameLine(200); ImGui::InputFloat("##initVel", &projectile_parameters["Initial Velocity"].value, 0.f, 0.f, "%.2f");
    ImGui::Text("Final Velocity (m/s):");      ImGui::SameLine(200); ImGui::InputFloat("##finalVel", &projectile_parameters["Final Velocity"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Initial Speed (m/s):");       ImGui::SameLine(200); ImGui::InputFloat("##initSpeed", &projectile_parameters["Initial Speed"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Final Speed (m/s):");         ImGui::SameLine(200); ImGui::InputFloat("##finalSpeed", &projectile_parameters["Final Speed"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Initial Height (m):");        ImGui::SameLine(200); ImGui::InputFloat("##initHeight", &projectile_parameters["Initial Height"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Friction (μ):");              ImGui::SameLine(200); ImGui::InputFloat("##friction", &projectile_parameters["coeff Friction"].value, 0.0f, 0.0f, "%.3f");
    ImGui::Text("X-Acceleration (m/s²):");     ImGui::SameLine(200); ImGui::InputFloat("##xAccel", &projectile_parameters["X Acceleration"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Y-Acceleration (m/s²):");     ImGui::SameLine(200); ImGui::InputFloat("##yAccel", &projectile_parameters["Y Acceleration"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Initial Force (N):");         ImGui::SameLine(200); ImGui::InputFloat("##initForce", &projectile_parameters["Initial Force"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Final Force (N):");           ImGui::SameLine(200); ImGui::InputFloat("##finalForce", &projectile_parameters["Final Force"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Launch Angle (°):");          ImGui::SameLine(200); ImGui::InputFloat("##angle", &projectile_parameters["WRT x Angle"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Time (s):");                  ImGui::SameLine(200); ImGui::InputFloat("##time", &projectile_parameters["Time Elapsed"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Mass (kg):");                 ImGui::SameLine(200); ImGui::InputFloat("##mass", &projectile_parameters["Mass"].value, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Distance (m):");              ImGui::SameLine(200); ImGui::InputFloat("##distance", &projectile_parameters["Distance"].value, 0.0f, 0.0f, "%.2f");

    // --- Results Variables ---
    static float maxHeight  = 0.0f;
    static float totalRange = 0.0f;
    static float flightTime = 0.0f;
    // --- Layout Settings ---
    ImGui::PushItemWidth(120.0f);
    // --- Results Section (read-only input boxes) ---
    ImGui::BeginDisabled();  // prevents user editing but still shows the inputs normally
    // --- Output Fields ---
    ImGui::Text("Maximum Height (m):"); ImGui::SameLine(200);
    ImGui::InputFloat("##maxHeight", &maxHeight, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Total Range (m):");    ImGui::SameLine(200);
    ImGui::InputFloat("##range", &totalRange, 0.0f, 0.0f, "%.2f");
    ImGui::Text("Time of Flight (s):"); ImGui::SameLine(200);
    ImGui::InputFloat("##timeFlight", &flightTime, 0.0f, 0.0f, "%.2f");
    //--- End of Results Section ---
    ImGui::EndDisabled();
    ImGui::PopItemWidth();
    // --- End of Results Tab ---
    ImGui::End();

    // Set initial position of the window
    ImGui::SetNextWindowPos(ImVec2(10.f,490.f), ImGuiCond_Once);
    // --- Trajectory Data Tab ---
    ImGui::Begin("Trajectory Data");
    // Dummy data for now — your program will fill this later
    static std::vector<float> timeValues = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static std::vector<float> xValues    = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static std::vector<float> yValues    = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    // --- Layout Settings ---
    ImGui::PushItemWidth(120.0f);
    // Playback controls
    ImGui::Button("<<"); 
    ImGui::SameLine();
    ImGui::Button("Play/Pause"); 
    ImGui::SameLine();
    ImGui::Button(">>");
    // Create table with borders and row backgrounds
    if (ImGui::BeginTable("TrajectoryTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Time (s)");
        ImGui::TableSetupColumn("X Distance (m)");
        ImGui::TableSetupColumn("Y Distance (m)");
        ImGui::TableHeadersRow();
        // Fill rows with data
        for (size_t i = 0; i < timeValues.size(); i++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f", timeValues[i]);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", xValues[i]);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", yValues[i]);
        }
        ImGui::EndTable(); 
    }
    ImGui::End();
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
    render_gui();

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
