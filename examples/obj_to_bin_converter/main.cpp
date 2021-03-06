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
#include <memory>
#include <algorithm>

#include <gua/guacamole.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
 
template <class T>
inline std::string
toString(T value){
    std::ostringstream stream;
    stream << value;
    return stream.str();
}


int main(int argc, char** argv)
{
    std::string obj_file = "";
    std::string bin_file_base = "";
    if(3 != argc) {
      std::cout << "usage: " << argv[0] << " filename.obj binfilename_base" << std::endl;
      return 0;  
    }
    obj_file = argv[1];
    bin_file_base = argv[2];

    char* argv_tmp[] = {"./example-obj_to_bin_converter", nullptr};
    int argc_tmp = sizeof(argv_tmp) / sizeof(char*) - 1;
    
    // initialize guacamole
    gua::init(argc_tmp, argv_tmp);

    bool make_pickable = false;
    if(const char* make_pickable_p = std::getenv("GUACAMOLE_MAKE_PICKABLE")){
        std::string make_pickable_flag(make_pickable_p);
        make_pickable = "TRUE" == make_pickable_flag ? true : false;

    }
    if(make_pickable){
        std::cout << "make pickable enabled, KDtree will be built and saved" << std::endl;        
    }
    

    std::cout << "start loading " << obj_file << std::endl;
    gua::TriMeshLoader loader;

    auto obj_node(loader.create_geometry_from_file("model_node" /*should be unique*/ , obj_file.c_str(), make_pickable ? gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::MAKE_PICKABLE : gua::TriMeshLoader::LOAD_MATERIALS));

    
    unsigned mesh_counter = 0;
    if(obj_node->get_children().empty()){
        std::string filename_gua_trimesh(bin_file_base + "_" + toString(mesh_counter) + ".gua_trimesh");
        std::string filename_gua_kdtree(bin_file_base + "_" + toString(mesh_counter) + ".gua_kdtree");
        auto t_node(std::dynamic_pointer_cast<gua::node::TriMeshNode>(obj_node));
        t_node->get_geometry()->save_to_binary(obj_file,(const char*) filename_gua_trimesh.c_str(), make_pickable ? (const char*) filename_gua_kdtree.c_str() : nullptr, gua::TriMeshLoader::LOAD_MATERIALS /*, gua::TriMeshRessource::SAVE_TANGENTS | gua::TriMeshRessource::SAVE_BITANGENTS*/);

        ++mesh_counter;
        std::cout << "saved " << filename_gua_trimesh << std::endl;
        if(make_pickable) std::cout << "saved " << filename_gua_kdtree << std::endl;
    }
    for(unsigned i = 0; i < obj_node->get_children().size(); ++i){
        std::string filename_gua_trimesh(bin_file_base + "_" + toString(mesh_counter) + ".gua_trimesh");
        std::string filename_gua_kdtree(bin_file_base + "_" + toString(mesh_counter) + ".gua_kdtree");
        auto t_node(std::dynamic_pointer_cast<gua::node::TriMeshNode>(obj_node->get_children()[i]));
        t_node->get_geometry()->save_to_binary(obj_file, (const char*) filename_gua_trimesh.c_str(), make_pickable ? (const char*) filename_gua_kdtree.c_str() : nullptr, gua::TriMeshLoader::LOAD_MATERIALS /*, gua::TriMeshRessource::SAVE_TANGENTS | gua::TriMeshRessource::SAVE_BITANGENTS*/);

        ++mesh_counter;
        std::cout << "saved " << filename_gua_trimesh << std::endl;
        if(make_pickable) std::cout << "saved " << filename_gua_kdtree << std::endl;
    }

    return 0;
}

