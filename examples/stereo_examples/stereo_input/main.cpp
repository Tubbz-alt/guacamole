/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2013 Bauhaus-Universität Weimar                        *
 * Contact:   felix.lauer@uni-weimar.de / simon.schneegans@uni-weimar.de      *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify it    *
 * under the terms of the GNU General Public License as published by the Free *
 * Software Foundation, either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program. If not, see <http://www.gnu.org/licenses/>.             *
 *                                                                            *
 ******************************************************************************/

#include <functional>

#include <gua/guacamole.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/renderer/ToneMappingPass.hpp>
#include <gua/renderer/BBoxPass.hpp>
#include <gua/renderer/DebugViewPass.hpp>
#include <gua/utils/Logger.hpp>
#include <gua/utils/Trackball.hpp>

// forward mouse interaction to trackball
void mouse_button(gua::utils::Trackball& trackball, int mousebutton, int action, int mods)
{
    gua::utils::Trackball::button_type button;
    gua::utils::Trackball::state_type state;

    switch(mousebutton)
    {
    case 0:
        button = gua::utils::Trackball::left;
        break;
    case 2:
        button = gua::utils::Trackball::middle;
        break;
    case 1:
        button = gua::utils::Trackball::right;
        break;
    };

    switch(action)
    {
    case 0:
        state = gua::utils::Trackball::released;
        break;
    case 1:
        state = gua::utils::Trackball::pressed;
        break;
    };

    trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

void adjust_arguments(int& argc, char**& argv)
{
    char* argv_tmp[] = {argv[0], NULL};
    int argc_tmp = sizeof(argv_tmp) / sizeof(char*) - 1;
    argc = argc_tmp;
    argv = argv_tmp;
}

enum class Side_By_Side_Mode {
  DEFAULT_SIDE_BY_SIDE = 0,
  SOFTWARE_MULTI_VIEW_RENDERING = 1,
  HARDWARE_MULTI_VIEW_RENDERING = 2,
  NUM_SIDE_BY_SIDE_MODES = 3
};


Side_By_Side_Mode sbs_mode = Side_By_Side_Mode::DEFAULT_SIDE_BY_SIDE;

std::string parse_model_from_cmd_line(int argc, char** argv)
{
    std::string model_path = "data/objects/teapot.obj";

    std::string log_message_model_string = "";
    if(argc < 2)
    {
        gua::Logger::LOG_MESSAGE << argv[0] << ": Did not provide any model file." << std::endl;
        log_message_model_string = "Using default model path: " + model_path;
    }
    else
    {
        model_path = argv[1];
        log_message_model_string = std::string(argv[0]) + ": Using provided model path:";

        if(argc > 2) {

            sbs_mode = Side_By_Side_Mode(std::atoi(argv[2]));
            gua::Logger::LOG_MESSAGE << "Setting side by side mode to " << (int(sbs_mode) == 0 ? " SIDE_BY_SIDE " : "SIDE_BY_SIDE_SOFTWARE_MULTI_VIEW_RENDERING") << std::endl; 
        }

    }
    gua::Logger::LOG_MESSAGE << log_message_model_string + model_path << std::endl;

    return model_path;
}

int main(int argc, char** argv)
{
    std::string model_path = parse_model_from_cmd_line(argc, argv);

    adjust_arguments(argc, argv);
    gua::init(argc, argv);

    // setup scene
    gua::SceneGraph graph("main_scenegraph");

    gua::TriMeshLoader loader;

    auto model_mat(gua::MaterialShaderDatabase::instance()->lookup("gua_default_material")->make_new_material());

    model_mat->set_render_wireframe(false);
    model_mat->set_show_back_faces(false);

    auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
    auto example_model(
        loader.create_geometry_from_file("example_model", model_path, model_mat, gua::TriMeshLoader::NORMALIZE_POSITION | gua::TriMeshLoader::NORMALIZE_SCALE | gua::TriMeshLoader::LOAD_MATERIALS));

    graph.add_node("/transform", example_model);
    example_model->set_draw_bounding_box(true);

    std::cout << "Hierarchy below teapot contains: " << example_model->num_grouped_faces() << " Triangles" << std::endl;

    auto portal = graph.add_node<gua::node::TexturedQuadNode>("/", "portal");
    portal->data.set_size(gua::math::vec2(1.2f, 0.8f));
    portal->data.set_texture("portal");
    portal->translate(0.5f, 0.f, -0.2f);
    portal->rotate(-30, 0.f, 1.f, 0.f);


    auto light2 = graph.add_node<gua::node::LightNode>("/", "light2");
    light2->data.set_type(gua::node::LightNode::Type::POINT);
    light2->data.brightness = 150.0f;
    light2->scale(12.f);
    light2->translate(-3.f, 5.f, 5.f);

    auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
    screen->data.set_size(gua::math::vec2(1.92f, 1.08f));
    screen->translate(0, 0, 1.0);

    auto portal_screen = graph.add_node<gua::node::ScreenNode>("/", "portal_screen");
    portal_screen->translate(0.0, 0.0, 5.0);
    portal_screen->rotate(90, 0.0, 1.0, 0.0);
    portal_screen->data.set_size(gua::math::vec2(1.2f, 0.8f));

    // add mouse interaction
    gua::utils::Trackball trackball(0.01, 0.002, 0.2);

    // setup rendering pipeline and window
    auto resolution = 1.0 * gua::math::vec2ui(1.8*960, 1.8*540);

    auto portal_camera = graph.add_node<gua::node::CameraNode>("/portal_screen", "portal_cam");
    portal_camera->translate(0, 0, 2.0);
    portal_camera->config.set_resolution(gua::math::vec2ui(1200, 800));
    portal_camera->config.set_screen_path("/portal_screen");
    portal_camera->config.set_scene_graph_name("main_scenegraph");
    portal_camera->config.set_output_texture_name("portal");
    portal_camera->config.set_enable_stereo(false);

    auto portal_pipe = std::make_shared<gua::PipelineDescription>();
    portal_pipe->add_pass(std::make_shared<gua::TriMeshPassDescription>());
    portal_pipe->add_pass(std::make_shared<gua::LightVisibilityPassDescription>());

    auto resolve_pass = std::make_shared<gua::ResolvePassDescription>();
    resolve_pass->background_mode(gua::ResolvePassDescription::BackgroundMode::QUAD_TEXTURE);
    resolve_pass->tone_mapping_exposure(1.0f);

    portal_pipe->add_pass(resolve_pass);
    portal_pipe->add_pass(std::make_shared<gua::DebugViewPassDescription>());

    portal_camera->set_pipeline_description(portal_pipe);

    auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
    camera->translate(0, 0, 2.0);
    camera->config.set_resolution(resolution);
    camera->config.set_left_screen_path("/screen");
    camera->config.set_right_screen_path("/screen");
    camera->config.set_scene_graph_name("main_scenegraph");
    camera->config.set_output_window_name("main_window");
    camera->config.set_enable_stereo(true);
    //camera->set_pre_render_cameras({portal_camera});

    auto default_pipeline_description = std::make_shared<gua::PipelineDescription>();
    default_pipeline_description->add_pass(std::make_shared<gua::TriMeshPassDescription>());
    default_pipeline_description->add_pass(std::make_shared<gua::BBoxPassDescription>());
    default_pipeline_description->add_pass(std::make_shared<gua::LightVisibilityPassDescription>());
    default_pipeline_description->add_pass(std::make_shared<gua::ResolvePassDescription>());
    default_pipeline_description->add_pass(std::make_shared<gua::DebugViewPassDescription>());
    camera->set_pipeline_description(default_pipeline_description);



    auto window = std::make_shared<gua::GlfwWindow>();
    window->config.set_enable_vsync(false);
    window->config.set_size(scm::math::vec2ui(resolution.x * 2, resolution.y ) ) ;
    window->config.set_resolution(scm::math::vec2ui(resolution.x * 2, resolution.y ) );
    window->config.set_right_position(scm::math::vec2ui(resolution.x , 0));
    window->config.set_left_resolution(scm::math::vec2ui(resolution.x , resolution.y));
    window->config.set_right_resolution(scm::math::vec2ui(resolution.x , resolution.y));

    if( 0 == int(sbs_mode) ) {
        window->config.set_stereo_mode(gua::StereoMode::SIDE_BY_SIDE);
    } else if( 1 == int(sbs_mode) ) {
        window->config.set_stereo_mode(gua::StereoMode::SIDE_BY_SIDE_SOFTWARE_MULTI_VIEW_RENDERING);
    } else if( 2 == int(sbs_mode) ) {
        window->config.set_stereo_mode(gua::StereoMode::SIDE_BY_SIDE_HARDWARE_MULTI_VIEW_RENDERING);
    }

    window->on_resize.connect([&](gua::math::vec2ui const& new_size) {
        window->config.set_resolution(new_size);
        camera->config.set_resolution(new_size);
        screen->data.set_size(gua::math::vec2(0.001 * new_size.x, 0.001 * new_size.y));
    });
    window->on_move_cursor.connect([&](gua::math::vec2 const& pos) { trackball.motion(pos.x, pos.y); });
    window->on_button_press.connect(std::bind(mouse_button, std::ref(trackball), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));





    gua::Renderer renderer;

    // application loop
    gua::events::MainLoop loop;

    int cnt = 0;


    gua::events::Ticker ticker(loop, 1.0 / 5000.0);


    double frame_time_avg = 0.0;
    double last_frame_time = -1.0;
    
    uint32_t valid_frames_recorded = 0;
    uint32_t waiting_frames_recorded = 0;
    uint32_t const num_frames_to_average = 1000;
    uint32_t const waiting_frames = num_frames_to_average/10;
    ticker.on_tick.connect([&]() {
        // apply trackball matrix to object
        gua::math::mat4 modelmatrix = scm::math::make_translation(trackball.shiftx(), trackball.shifty(), trackball.distance()) * gua::math::mat4(trackball.rotation());

        transform->set_transform(modelmatrix);
        ++cnt;
        if(cnt == 1) {
           gua::WindowDatabase::instance()->add("main_window", window);
        }

        if(window->get_rendering_fps() > 0) {
          if(valid_frames_recorded < num_frames_to_average) {
            double current_frame_time = 1.0 / window->get_rendering_fps();
            if(last_frame_time != current_frame_time) {
              //std::cout << "draw time: " << 1.0 / window->get_rendering_fps() << std::endl;

              last_frame_time = current_frame_time;

              if(waiting_frames_recorded < waiting_frames) {
                ++waiting_frames_recorded;
              } else {
                frame_time_avg += current_frame_time;
                ++valid_frames_recorded;
              }
            }
          } else if(num_frames_to_average == valid_frames_recorded) {
            std::cout << "avg frame time: " << frame_time_avg / valid_frames_recorded << std::endl;
            ++valid_frames_recorded;
          }
        }



        if(window->should_close())
        {
            renderer.stop();
            window->close();
            loop.stop();
        }
        else
        {
            renderer.queue_draw({&graph});
        }
    });

    loop.start();

    return 0;
}