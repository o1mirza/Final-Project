// Imports
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <map>
#include <algorithm>

// GLOBAL VALUES
// Constant config values
const unsigned int FPS_LOCK {60}, WIDTH {1280}, HEIGHT {720};
const double CAMERA_SPEED {2.0}, TIME_INTERVAL{0.1};

// GLOBAL VARIABLES
bool stop_time {true}, is_solved {false}, follow_projectile {false};
std::string user_error_message {};

// Main rendering surface 
sf::RenderWindow *window = nullptr;
// Camera object - perspective of the user
sf::View camera(sf::FloatRect({0, 0}, {WIDTH, HEIGHT}));

// 2D Vector
struct Vector2 {
    double x, y;

    Vector2(): x(0.f), y(0.f) {}
    Vector2(double i, double j): x(i), y(j) {}
    Vector2(int i, int j): x(static_cast<double>(i)), y(static_cast<double>(j)) {}
};

// Enums to store parameter name and table
enum class Parameter{V_INITIAL_I_COMPONENT, V_INITIAL_J_COMPONENT, V_FINAL_I_COMPONENT, V_FINAL_J_COMPONENT, Y_INITIAL, ACC, ANGLE, TIME, RANGE, ABS_MAX_HEIGHT, MAX_HEIGHT, TIME_OF_APEX, INITIAL_SPEED, FINAL_SPEED, COEFF_FRICTION, FORCE, MASS};
enum class ParameterTable {KINEMATICS_SCALAR, KINEMATICS_VECTOR, FORCES, BOTH}; // Both is kinematic and scalar 

// Holds the value and corresponding metadata about the parameter
struct ParameterInfo{
    std::string name {};

    double value {};
    const double default_value {};
    const int min {}, max {};
    
    const bool is_required {};
    ParameterTable info_type {};
    std::vector<Parameter> dependencies {};
};

// Map to store user input and values to be displayed to the user
std::map<Parameter, ParameterInfo> projectile_parameters {
    {Parameter::V_INITIAL_I_COMPONENT, ParameterInfo{"v_initial_i_component", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_VECTOR, {Parameter::V_INITIAL_J_COMPONENT}}},
    {Parameter::V_INITIAL_J_COMPONENT, ParameterInfo{"v_ininitial_j_component", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_VECTOR, {Parameter::V_INITIAL_I_COMPONENT}}},
    {Parameter::V_FINAL_I_COMPONENT, ParameterInfo{"v_final_i_component", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_VECTOR, {Parameter::V_FINAL_J_COMPONENT}}},
    {Parameter::V_FINAL_J_COMPONENT, ParameterInfo{"v_final_j_component", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_VECTOR, {Parameter::V_FINAL_I_COMPONENT}}},
    {Parameter::INITIAL_SPEED, ParameterInfo{"initial_speed", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_SCALAR, {}}},
    {Parameter::FINAL_SPEED, ParameterInfo{"final_speed", 0.f, 0.f, 1, 1000, true, ParameterTable::KINEMATICS_SCALAR, {}}},
    {Parameter::Y_INITIAL, ParameterInfo{"y_initial", 0.f, 0.f, 0, 1000, false, ParameterTable::BOTH, {}}},
    {Parameter::COEFF_FRICTION, ParameterInfo{"coeff_friction", 0.f, 0.f, 1, 1, true, ParameterTable::FORCES, {Parameter::FORCE, Parameter::TIME, Parameter::MASS}}},
    {Parameter::ACC, ParameterInfo{"acc", 0.f, 0.f, -1000, -1, true, ParameterTable::BOTH, {}}},
    {Parameter::FORCE, ParameterInfo{"force", 0.f, 0.f, 1, 1000, true, ParameterTable::FORCES, {Parameter::TIME, Parameter::MASS}}},
    {Parameter::ANGLE, ParameterInfo{"angle", 45.f, 45.f, 0, 90, false, ParameterTable::BOTH, {}}},
    {Parameter::TIME, ParameterInfo{"time", 0.f, 0.f, 1, 1000, true, ParameterTable::BOTH, {}}},
    {Parameter::MASS, ParameterInfo{"mass", 0.f, 0.f, 1, 1000, true, ParameterTable::FORCES, {Parameter::FORCE, Parameter::TIME}}},
    {Parameter::RANGE, ParameterInfo{"range", 0.f, 0.f, 1, 1000, true, ParameterTable::BOTH, {}}},
    {Parameter::MAX_HEIGHT, ParameterInfo{"max_height", 0.f, 0.f, 1, 1000, true, ParameterTable::BOTH, {}}},
    {Parameter::ABS_MAX_HEIGHT, ParameterInfo{"abs_max_height", 0.f, 0.f, 1, 1000, false, ParameterTable::BOTH, {}}},
    {Parameter::TIME_OF_APEX, ParameterInfo{"apexTime", 0.f, 0.f, 1, 1000, false, ParameterTable::BOTH, {}}}
};

// Physics Engine
void find_unknown(double &y_initial, double &v_initial, double &v_final, double &acc, double &time, double &max_height, double &abs_max_height, double &range, const double &angle, double &v_initial_i_component, double &v_initial_j_component, double &v_final_i_component, double &v_final_j_component, double &apexTime) {    
    // y_initial  --> initial height of the projectile with respect to the ground
    // v_initial  --> initial speed (non-vector) of projectile
    // v_final    --> final speed (non-vector) of projectile
    // acc        --> acceleration magnitude (non-vector) of projectile (vertical axis only)
    // time       --> full period of motion of projectile
    // max_height     --> Height / Vertical displacement of projectile
    // range      --> Horizontal Distance Travalled by the Projectile
    // angle      --> angle (degrees) with respect to the x-axis
    // v_initial_i_component  --> initial horiztonal velocity vector component (always greater than 0)
    // v_initial_j_component  --> initial vertical velocity vector component (either 0 or positive)
    // v_final_i_component    --> final horizontal velocity vector component (always greater than 0)
    // v_final_j_component    --> final vertical velocity component (either 0 or negative)

    double theta {}; // Launch Angle: Used for symmetrical case (y_initil = 0)
    //double theta1 {}; // Launch Angle: Used for asymmetrical case -- initial angle (y_initial > 0)
    //double theta2 {}; // Impact Angle: Used for asymmetrical case -- final angle (y_initial > 0)

    // The code below is responsible for checking and calculating two unknown parameters of the projectile
    // by using the 3 known parameters. The three known parameters may be used to calculate both of the 
    // remaining parameters, or solving for one can be further used to solve for the other.

    if(y_initial == 0.0) { // instructions executed if projectile begins on the ground

        // Normalizing vector inputs below

        if(v_initial_i_component != 0) { // only checking for initial i-component
            v_initial = sqrt(pow(v_initial_i_component, 2) + pow(v_initial_j_component, 2));

            theta = std :: atan(abs(v_initial_j_component / v_initial_i_component));
        }

        else {theta = angle * (M_PI / 180.0);}

        if(v_final_i_component != 0) { // only checking for final i-component
            v_final = sqrt(pow(v_final_i_component, 2) + pow(v_final_j_component, 2));

            theta = std::atan(abs(v_final_j_component / v_final_i_component));
        }

        else {theta = angle * (M_PI / 180.0);}

        // 1. time & max_height & range (checked)
        if(time == 0 && max_height == 0 && range == 0) {
            time = (((0 - v_initial) * std::sin(theta)) / acc) * 2;
            max_height = (v_initial * std::sin(theta) * (time / 2)) + (0.5 * acc * pow(time / 2, 2));
            range = v_initial * std::cos(theta) * time;
        }

        // 2. time & acc & range (checked)
        else if(time == 0 && acc == 0 && range == 0) {
            acc = (0 - pow((v_initial * std::sin(theta)), 2)) / (2 * max_height);
            time = (((0 - v_initial) * std::sin(theta)) / acc) * 2;
            range = v_initial * std::cos(theta) * time;
        }

        // 3. acc & v_initial & range (checked)
        else if(acc == 0 && v_initial == 0 && range == 0) {
            v_initial = v_final;
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            range = v_initial * std::cos(theta) * time;
        }

        // 4. v_final & acc & range (checked)
        else if(v_final == 0 && acc == 0 && range == 0) {
            v_final = v_initial;
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            range = v_initial * std::cos(theta) * time;
        }

        // 5. v_final & time & range (checked)
        else if(v_final == 0 && time == 0 && range == 0) {
            v_final = v_initial;
            time = ((0 - v_initial) * std::sin(theta)) / (acc / 2);
            range = v_initial * std::cos(theta) * time;
        }

        // 6. v_inital & time & range (checked)
        else if(v_initial == 0 && time == 0 && range == 0) {
            v_initial = v_final;
            time = (((0 - v_initial) * std::sin(theta)) / acc) * 2;
            range = v_initial * std::cos(theta) * time;
        }

        // 7. max_height & acc & range (checked)
        else if(max_height == 0 && acc == 0 && range == 0) {
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            max_height = -pow((5 * std::sin(theta)), 2) / acc;
            range = v_initial * std::cos(theta) * time;
        }

        // 8. v_initial & range & max_height (checked)
        else if(v_initial == 0 && range == 0 && max_height == 0) {
            v_initial = v_final;
            time = range / (v_initial * std::cos(theta));
            max_height = (v_initial * std::sin(theta) * (time / 2)) + (0.5 * acc * pow((time / 2), 2));
        }

        // 9. range & v_final & max_height (checked)
        else if(range == 0 && v_final == 0 && max_height == 0) {
            v_final  = v_initial;
            range = v_initial * std::cos(theta) * time;
            max_height = (v_initial * std::sin(theta) * (time / 2)) + (0.5 * acc * pow((time / 2), 2));
        }

        // 10. v_final & v_initial & range ()
        else if(v_final == 0 && v_initial == 0 && range == 0) {
            v_initial = (max_height - 0.5 * acc * pow(time / 2, 2)) / ((time / 2) * std::sin(theta));
            v_final = v_initial;
            range = v_initial * std::cos(theta) * time;
        }

        // 11. max_height & time & v_initial (checked)
        else if(max_height == 0 && time == 0 && v_initial == 0) {
            v_initial = v_final;
            time = (((0 - v_initial) * std::sin(theta)) / acc) * 2;
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 12. max_height & time & v_final (checked)
        else if(max_height == 0 && time == 0 && v_final == 0) {
            v_final = v_initial;
            time = (((0 - v_initial) * std::sin(theta) * 2) / acc) * 2;
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 13. max_height & time & acc (checked)
        else if(max_height == 0 && time == 0 && acc == 0) {
            time = range / (v_initial * std::cos(theta));
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 14. max_height & v_initial & v_final (checked)
        else if(max_height == 0 && v_initial == 0 && v_final == 0) {
            v_initial = range / (time * std::cos(theta));
            v_final = v_initial;
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 15. max_height & v_initial & acc (checked)
        else if(max_height == 0 && v_initial == 0 && acc == 0) {
            v_initial = v_final;
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 16. max_height & v_final & acc (checked)
        else if(max_height == 0 && v_final == 0 && acc == 0) {
            v_final = v_initial;
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
            max_height = (v_initial * std::sin(theta) * time) + (0.5 * acc * pow(time, 2));
        }

        // 17. time & v_initial & v_final (checked)
        else if(time == 0 && v_initial == 0 && v_final == 0) {
            time = 0;
            v_initial = 0;
            v_final = 0;
        }

        // 18. time & v_initial & acc (checked)
        else if(time == 0 && v_initial == 0 && acc == 0) {
            v_initial = v_final;
            time = range / (v_initial * std::cos(theta));
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
        }

        // 19. time & v_final & acc (checked)
        else if(time == 0 && v_final == 0 && acc == 0) {
            v_final = v_initial;
            time = range / (v_initial * std::cos(theta));
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
        }

        // 20. v_inital & v_final & acc (checked)
        else if(v_initial == 0 && v_final == 0 && acc == 0) {
            v_initial = range / (time * std::cos(theta));
            v_final = v_initial;
            acc = ((0 - v_initial) * std::sin(theta)) / (time / 2);
        }
    }

    abs_max_height = y_initial + max_height; // Calculate maximum height (absolute) with respect to ground
    apexTime = (-1 * v_initial * std::sin(theta)) / acc / 2; // Calculate or recalculate time the projectile needs to reach maximum height with respect to launch
    is_solved = true;
    user_error_message = "";
}

// Verifies all input fields
void cleanup_input(){
    unsigned int required_scalar_count {}, required_vector_count {};
    std::vector<int> given {};
    is_solved = false;

    // Verify all values are inside their ranges
    for (const auto &[name, info] : projectile_parameters){
        if (info.value == 0.0) // Ignore default values
            continue;

        if (info.value < info.min || info.value > info.max){
            user_error_message = "Parameter: " + info.name +
                     " with value: " + std::to_string(info.value) +
                     " is not within allowed range [" + std::to_string(info.min) + ", " +
                     std::to_string(info.max) + "]\nPlease enter valid and consistent values!";
            return;
        }

        // Cache to check for dependencies later
        given.push_back(static_cast<int>(name));
    }

    // Check that all of the given variables have the required dependencies
    for (int &index : given){
        const ParameterInfo &info = projectile_parameters[static_cast<Parameter>(index)];
        for (const Parameter &dep : info.dependencies) {
            if (std::find(given.begin(), given.end(), static_cast<int>(dep)) == given.end()) {
                user_error_message = "Missing required dependency for parameter: " + 
                          info.name + ": dependency " +
                          projectile_parameters[dep].name + " not provided";
                return;
            }
        }

        if (info.is_required) {
            if (info.info_type == ParameterTable::KINEMATICS_SCALAR || info.info_type == ParameterTable::BOTH)
                required_scalar_count++;

            if (info.info_type == ParameterTable::KINEMATICS_VECTOR || info.info_type == ParameterTable::BOTH)
                required_vector_count++;
        }
    }

    // Check if the threshold is reached and call the physics engine
    if (required_scalar_count >= 3 || required_vector_count >= 3){
        // Verify initial and final velocities are the same if both are given
        if(projectile_parameters[Parameter::INITIAL_SPEED].value != 0.f && projectile_parameters[Parameter::FINAL_SPEED].value != 0.f) {
            if (projectile_parameters[Parameter::INITIAL_SPEED].value != projectile_parameters[Parameter::FINAL_SPEED].value){
                user_error_message = "Initial and Final speed are NOT the same!";
                return;
            }
        }

        if(projectile_parameters[Parameter::V_INITIAL_I_COMPONENT].value != 0.f && projectile_parameters[Parameter::V_FINAL_I_COMPONENT].value != 0.f) {
            if(projectile_parameters[Parameter::V_INITIAL_I_COMPONENT].value != projectile_parameters[Parameter::V_FINAL_I_COMPONENT].value) {
                user_error_message = "Initial and Final vertical (i) components are NOT the same!";
                return;
            }
        }

        if(projectile_parameters[Parameter::V_INITIAL_J_COMPONENT].value != 0.f && projectile_parameters[Parameter::V_FINAL_J_COMPONENT].value != 0.f) {
            if(projectile_parameters[Parameter::V_INITIAL_J_COMPONENT].value != projectile_parameters[Parameter::V_FINAL_J_COMPONENT].value) {
                user_error_message = "Initial and Final Horizontal (j) components are NOT the same!";
                return;
            }
        }

        // Inputs look valid, call the physics engine
        find_unknown(
            projectile_parameters[Parameter::Y_INITIAL].value,
            projectile_parameters[Parameter::INITIAL_SPEED].value,
            projectile_parameters[Parameter::FINAL_SPEED].value,
            projectile_parameters[Parameter::ACC].value,
            projectile_parameters[Parameter::TIME].value,
            projectile_parameters[Parameter::MAX_HEIGHT].value,
            projectile_parameters[Parameter::ABS_MAX_HEIGHT].value,
            projectile_parameters[Parameter::RANGE].value,
            projectile_parameters[Parameter::ANGLE].value,
            projectile_parameters[Parameter::V_INITIAL_I_COMPONENT].value,
            projectile_parameters[Parameter::V_INITIAL_J_COMPONENT].value,
            projectile_parameters[Parameter::V_FINAL_I_COMPONENT].value,
            projectile_parameters[Parameter::V_FINAL_J_COMPONENT].value,
            projectile_parameters[Parameter::TIME_OF_APEX].value
        );

        // Check output value -> <0 means the input values lead to an impossible case
        if(projectile_parameters[Parameter::MAX_HEIGHT].value  < 0) {
            user_error_message = "User has entered inconsistent values!\nLook at the calculated value of Maximum Height!";
            is_solved = false;
            return;
        }
    } 

    else {
        user_error_message = "Not enough required inputs. Required scalar count = " + std::to_string(required_scalar_count) +
                  ", required vector count = " + std::to_string(required_vector_count);
    }
}

// Static object handler
using shape_ptr = std::shared_ptr<sf::Shape>; // Bit cleaner to use shape_ptr instead of having to type all of that
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

        void move(const shape_ptr &obj, const Vector2 &d_pos){
            obj->move({d_pos.x, d_pos.y});
        }

};
// Initialize the dynamic object handler
dynamic_object_manager dynamic_object_handler {};

// Projectile Class
class projectile_manager {
    private:
        shape_ptr object_ptr;
        double t {0};
        const double radius {30};

    public:
        std::vector<Vector2> pos_list;
        const double start_x{15}, start_y{30};
        double x, y;

        // Normalizes coords to the bottom right of the screen
        void normalize_coords(double &y){
            double height = static_cast<float>(window->getSize().y);
            y = height - y;
        }


        projectile_manager(){
            this-> x = this->start_x;
            this->y = start_y;
            normalize_coords(this->y);

            this->pos_list.push_back(Vector2(this->x, this->y));
            auto circle = sf::CircleShape{this->radius};
            circle.setOrigin({this->radius, this->radius});
            this->object_ptr = dynamic_object_handler.add_object(circle, sf::Color::Red, Vector2(this->x, this->y));
        }

    void move(){
        this->t += TIME_INTERVAL;
        double angle_rad = projectile_parameters[Parameter::ANGLE].value * (M_PI / 180);
        double v_initial_x {}, v_initial_y {};

        if (projectile_parameters[Parameter::INITIAL_SPEED].value != 0){
            v_initial_x = projectile_parameters[Parameter::INITIAL_SPEED].value * std::cos(angle_rad);
            v_initial_y = projectile_parameters[Parameter::INITIAL_SPEED].value * std::sin(angle_rad);
        }
        else{
            v_initial_x = projectile_parameters[Parameter::V_INITIAL_I_COMPONENT].value;
            v_initial_y = projectile_parameters[Parameter::V_INITIAL_J_COMPONENT].value;
        }

        double new_x = this->start_x + (v_initial_x * this->t);
        double new_y = this->start_y + (v_initial_y * this->t + 0.5 * projectile_parameters[Parameter::ACC].value * pow(this->t, 2));
        normalize_coords(new_y); // this->start_y

        double del_x = new_x - this->x; 
        double del_y = new_y - this->y;
        this->x += del_x; this->y += del_y; 

        double height = static_cast<float>(window->getSize().y);
        if (height - this->y < 0){;
            stop_time = true;
            this->y = this->start_y;
            normalize_coords(this->y);
        }

        dynamic_object_handler.move(this->object_ptr, Vector2(del_x, del_y));
        this->pos_list.push_back(Vector2(this->x, this->y));// So we can let the user more back and forwards in the frame
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

// Sets all the values in the map to their default values - runs when clear button is pressed or when the user switches tabs in the GUI
void clear_userInput(projectile_manager &main_projectile) {
    for (auto &[_, info] : projectile_parameters){
        info.value = info.default_value;
    }

    user_error_message = ""; // Clear error message string
    is_solved = false;
    stop_time = true;
    main_projectile.x = main_projectile.start_x;
    main_projectile.y = main_projectile.start_y;
    main_projectile.normalize_coords(main_projectile.y);
    main_projectile.pos_list.clear();
}

// Handles all of the keyboard interactions
void process_keyboard(){
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_P))) {
        stop_time = !stop_time;
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightAlt))) {
    }
}

void render_gui(projectile_manager &main_projectile){
    static ParameterTable table_state {}; // Holds which tab is currently selected, used for clearing values on switch

    // --- Input Tab ---
    // Set initial position of the window
    ImGui::SetNextWindowPos(ImVec2(10.f,10.f), ImGuiCond_Once);
    ImGui::Begin("Input Tab");

    // --- Layout Settings ---
    ImGui::PushItemWidth(120.0f);

    // Create tab bar
    if (ImGui::BeginTabBar("InputTabs"))
    {
        // Kinematics Tab
        if (ImGui::BeginTabItem("Kinematics")) {

            // Kinematics Sub Tabs
            if (ImGui::BeginTabBar("KinematicsSubTabs")) {
                if (ImGui::BeginTabItem("Scalar Values")) {
                     // --- Input Fields ---
                    if(table_state != ParameterTable::KINEMATICS_SCALAR) {
                        clear_userInput(main_projectile);
                        table_state = ParameterTable::KINEMATICS_SCALAR;
                    }

                    ImGui::Text("Initial Speed (m/s):");       ImGui::SameLine(200); ImGui::InputDouble("##initSpeed", &projectile_parameters[Parameter::INITIAL_SPEED].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Final Speed (m/s):");         ImGui::SameLine(200); ImGui::InputDouble("##finalSpeed", &projectile_parameters[Parameter::FINAL_SPEED].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Initial Height (m):");        ImGui::SameLine(200); ImGui::InputDouble("##initHeight", &projectile_parameters[Parameter::Y_INITIAL].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Y-Acceleration (m/s²):");     ImGui::SameLine(200); ImGui::InputDouble("##yAccel", &projectile_parameters[Parameter::ACC].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Launch Angle (°):");          ImGui::SameLine(200); ImGui::InputDouble("##angle", &projectile_parameters[Parameter::ANGLE].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Time (s):");                  ImGui::SameLine(200); ImGui::InputDouble("##time", &projectile_parameters[Parameter::TIME].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Distance (m):");              ImGui::SameLine(200); ImGui::InputDouble("##distance", &projectile_parameters[Parameter::RANGE].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Maximum Height (m):");        ImGui::SameLine(200); ImGui::InputDouble("##maxHeight", &projectile_parameters[Parameter::MAX_HEIGHT].value, 0.0f, 0.0f, "%.2f");

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Vector Values")) {
                    // --- Input Fields ---
                    if(table_state != ParameterTable::KINEMATICS_VECTOR) {
                        clear_userInput(main_projectile);
                        table_state = ParameterTable::KINEMATICS_VECTOR;
                    }

                    ImGui::Text("Initial i Velocity (m/s):");    ImGui::SameLine(200); ImGui::InputDouble("##initVel_i", &projectile_parameters[Parameter::V_INITIAL_I_COMPONENT].value, 0.f, 0.f, "%.2f");
                    ImGui::Text("Initial j Velocity (m/s):");    ImGui::SameLine(200); ImGui::InputDouble("##initVel_j", &projectile_parameters[Parameter::V_INITIAL_J_COMPONENT].value, 0.f, 0.f, "%.2f");
                    ImGui::Text("Final i Velocity (m/s):");      ImGui::SameLine(200); ImGui::InputDouble("##finalVel_i", &projectile_parameters[Parameter::V_FINAL_I_COMPONENT].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Final j Velocity (m/s):");      ImGui::SameLine(200); ImGui::InputDouble("##finalVel_j", &projectile_parameters[Parameter::V_FINAL_J_COMPONENT].value, 0.0f, 0.0f, "%.2f"); // Passing values kept same because j-components are always the same
                    ImGui::Text("Initial Height (m):");        ImGui::SameLine(200); ImGui::InputDouble("##initHeight", &projectile_parameters[Parameter::Y_INITIAL].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Y-Acceleration (m/s²):");     ImGui::SameLine(200); ImGui::InputDouble("##yAccel", &projectile_parameters[Parameter::ACC].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Time (s):");                  ImGui::SameLine(200); ImGui::InputDouble("##time", &projectile_parameters[Parameter::TIME].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Distance (m):");              ImGui::SameLine(200); ImGui::InputDouble("##distance", &projectile_parameters[Parameter::RANGE].value, 0.0f, 0.0f, "%.2f");
                    ImGui::Text("Maximum Height (m):");        ImGui::SameLine(200); ImGui::InputDouble("##maxHeight", &projectile_parameters[Parameter::MAX_HEIGHT].value, 0.0f, 0.0f, "%.2f");

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            // --- Layout Settings ---
            ImGui::PushItemWidth(120.0f);
            // --- Results Section (read-only input boxes) ---
            ImGui::BeginDisabled();  // prevents user editing but still shows the inputs normally
            // --- Output Fields ---
            ImGui::Text("Time of Apex (s):"); ImGui::SameLine(200);
            ImGui::InputDouble("##timeFlight", &projectile_parameters[Parameter::TIME_OF_APEX].value, 0.0f, 0.0f, "%.2f");
            //--- End of Results Section ---
            ImGui::EndDisabled();
            ImGui::PopItemWidth();

            ImGui::EndTabItem();
        }

        // Forces Tab
        if (ImGui::BeginTabItem("Forces")) {
            // --- Input Fields ---
            if(table_state != ParameterTable::FORCES) {
                clear_userInput(main_projectile);
                table_state = ParameterTable::FORCES;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 10.0f, 1.0f, 1.0f));
            ImGui::Text(" !!!!!!!!!!!!!!! Coming Soon !!!!!!!!!!!!!!!");
            ImGui::PopStyleColor();
            ImGui::BeginDisabled();  // prevents user editing but still shows the inputs normally
            ImGui::Text("Friction Coefficiant (Mu):");              ImGui::SameLine(200); ImGui::InputDouble("##friction", &projectile_parameters[Parameter::COEFF_FRICTION].value, 0.0f, 0.0f, "%.3f");
            ImGui::Text("Force (N):");         ImGui::SameLine(200); ImGui::InputDouble("##initForce", &projectile_parameters[Parameter::FORCE].value, 0.0f, 0.0f, "%.2f");
            ImGui::Text("Time (s):");                  ImGui::SameLine(200); ImGui::InputDouble("##time", &projectile_parameters[Parameter::TIME].value, 0.0f, 0.0f, "%.2f");
            ImGui::Text("Mass (kg):");                 ImGui::SameLine(200); ImGui::InputDouble("##mass", &projectile_parameters[Parameter::MASS].value, 0.0f, 0.0f, "%.2f");
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Text("Errors:");

        ImGui::BeginChild("ErrorBox", ImVec2(0, 60), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 10.0f, 1.0f, 1.0f));
        ImGui::TextWrapped("%s", user_error_message.c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();

        if(ImGui::Button("CALCULATE")) {
            cleanup_input();
        }

        ImGui::SameLine(100); if(ImGui::Button("CLEAR")) {
            clear_userInput(main_projectile);
        }
    }

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
    if (ImGui::Button(stop_time ? "Play" : "Pause")){
        if (!is_solved){
            user_error_message = "Can not play without values";

        }
        else{
            stop_time = !stop_time;
        }

    }
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

// Check if the projectile left the screen
bool is_projectile_off_screen(projectile_manager &main_projectile){
    return (main_projectile.x + 30 < 0 ||        // left
            main_projectile.x > WIDTH ||         // right
            main_projectile.y + 30 < 0 ||        // top
            main_projectile.y > HEIGHT);         // bottom
}

// Main window processing handler
void window_processing(projectile_manager &main_projectile){
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
    render_gui(main_projectile);

    // SFML Drawing
    // Set the user view as the camera
    window->clear(sf::Color::Black);
    follow_projectile = is_projectile_off_screen(main_projectile);
    if (false){// to do, follow projectile when it leaves the bounds of the camera
        double height = static_cast<float>(window->getSize().y);
        camera.setCenter({main_projectile.x, height - main_projectile.y});
    }
    window->setView(camera);

    // Move
    if (!stop_time)
        main_projectile.move();

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
        (desktop.size.x / 2 - window_obj.getSize().x / 2),
        (desktop.size.y / 2 - window_obj.getSize().y / 2)
    });

    // Check if the GUI is mounted
    if (!ImGui::SFML::Init(*window))
        return -1;

    // Add objects to the frame
    projectile_manager main_projectile {};

    // Draw some boxes in the bacground for scale
    sf::Color color;
    for (int x = 0; x < projectile_parameters[Parameter::RANGE].max * 100; x += 100) {
        // Alternate colors: even stripes white, odd stripes black
        if ((x/100) % 2 == 0){
            color = sf::Color(43, 81, 134);
            static_object_renderer.add_object(sf::RectangleShape({100.f, 1000.f}), color, Vector2(x, 0));
        }   
        // Dont need to draw black rectangle, background is already black so saves memory by not drawing unnecessary objects   
    }

    // While the window is open, run the main processing loop
    while (window->isOpen()){
        window_processing(main_projectile);
    }

    // Shutdown the ImGUI safely and end execution 
    ImGui::SFML::Shutdown();
    return 0;
}
