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

// class header
#include <gua/renderer/TriMeshRessource.hpp>

// guacamole headers
#include <gua/platform.hpp>
#include <gua/renderer/RenderContext.hpp>
#include <gua/renderer/ShaderProgram.hpp>
#include <gua/node/TriMeshNode.hpp>
#include <gua/utils/Logger.hpp>

// standard header
#include <fstream>

namespace gua
{
////////////////////////////////////////////////////////////////////////////////

TriMeshRessource::TriMeshRessource() : kd_tree_(), mesh_() {}

////////////////////////////////////////////////////////////////////////////////

TriMeshRessource::TriMeshRessource(Mesh const& mesh, bool build_kd_tree) : kd_tree_(), mesh_(mesh)
{
    if(mesh_.num_vertices > 0)
    {
        bounding_box_ = math::BoundingBox<math::vec3>();

        for(unsigned v(0); v < mesh_.num_vertices; ++v)
        {
            bounding_box_.expandBy(math::vec3{mesh_.positions[v]});
        }

        if(build_kd_tree)
        {
            if(!kd_tree_.generate(mesh_.base_filename.c_str())){
                kd_tree_.generate(mesh);
            }
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::upload_to(RenderContext& ctx) const
{
    RenderContext::Mesh cmesh{};
    cmesh.indices_topology = scm::gl::PRIMITIVE_TRIANGLE_LIST;
    cmesh.indices_type = scm::gl::TYPE_UINT;
    cmesh.indices_count = mesh_.num_triangles * 3;

    if(!(mesh_.num_vertices > 0))
    {
        Logger::LOG_WARNING << "Unable to load Mesh! Has no vertex data." << std::endl;
        return;
    }

    cmesh.vertices = ctx.render_device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, mesh_.num_vertices * sizeof(Mesh::Vertex), 0);

    Mesh::Vertex* data(static_cast<Mesh::Vertex*>(ctx.render_context->map_buffer(cmesh.vertices, scm::gl::ACCESS_WRITE_INVALIDATE_BUFFER)));

    mesh_.copy_to_buffer(data);

    ctx.render_context->unmap_buffer(cmesh.vertices);

    auto const max_index_iterator = std::max_element(mesh_.indices.begin(), mesh_.indices.end());
    
    uint8_t num_index_bits = 32;

    if(max_index_iterator != mesh_.indices.end()) {
        if( (1 <<  8) > *max_index_iterator) {
            num_index_bits = 8;
        }
        else if( (1 << 16) > *max_index_iterator) {
            num_index_bits = 16;
        }
    }

    uint32_t const num_indices = mesh_.num_triangles * 3;
    switch(num_index_bits) {
        /*case 8: {
            std::cout << "USING 8 BIT INDICES" << std::endl;
            cmesh.indices_type = scm::gl::TYPE_UBYTE;
            std::vector<uint8_t> eight_bit_indices(mesh_.indices.size());
            std::copy(mesh_.indices.begin(), mesh_.indices.end(), eight_bit_indices.begin());
            cmesh.indices = ctx.render_device->create_buffer(scm::gl::BIND_INDEX_BUFFER, scm::gl::USAGE_STATIC_DRAW,  num_indices * sizeof(uint8_t), eight_bit_indices.data());
            break;  
        }*/
        case 16: {
            std::cout << "USING 16 BIT INDICES" << std::endl;
            cmesh.indices_type = scm::gl::TYPE_USHORT;
            std::vector<uint16_t> sixteen_bit_indices(mesh_.indices.size());
            std::copy(mesh_.indices.begin(), mesh_.indices.end(), sixteen_bit_indices.begin());
            cmesh.indices = ctx.render_device->create_buffer(scm::gl::BIND_INDEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, num_indices * sizeof(uint16_t), sixteen_bit_indices.data());
            break;
        }

        default: {
            std::cout << "USING 32 BIT INDICES" << std::endl;
            cmesh.indices = ctx.render_device->create_buffer(scm::gl::BIND_INDEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, mesh_.num_triangles * 3 * sizeof(uint32_t), mesh_.indices.data());
            break;
        }
    }


    cmesh.vertex_array = ctx.render_device->create_vertex_array(mesh_.get_vertex_format(), {cmesh.vertices});
    ctx.meshes[uuid()] = cmesh;

    ctx.render_context->apply();
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::upload_kdtree_to(RenderContext& ctx) const
{
    RenderContext::BoundingBoxHierarchy cbounding_box_hierarchy{};
    cbounding_box_hierarchy.indices_topology = scm::gl::PRIMITIVE_TRIANGLE_STRIP;
    cbounding_box_hierarchy.indices_type = scm::gl::TYPE_UINT;
    //12 triangles per box, 3 vertices per triangle
    cbounding_box_hierarchy.indices_count =  kd_tree_.indices.size(); //render each box as triangle strip


    //should only happe if the KD-tree was not build in the first place
    if(!(kd_tree_.get_num_nodes() > 0))
    {
        Logger::LOG_WARNING << "Unable to load KD-Tree into GPU buffer! The KD-Tree does not contain nodes." << std::endl;
        return;
    }

    cbounding_box_hierarchy.vertices = ctx.render_device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, kd_tree_.get_num_nodes() * sizeof(float) * 3, 0);

    float* data(static_cast<float*>(ctx.render_context->map_buffer(cbounding_box_hierarchy.vertices, scm::gl::ACCESS_WRITE_INVALIDATE_BUFFER)));

    //std::cout << "KD TREE COPY TO BUFFER!" << std::endl;
    kd_tree_.copy_to_buffer(data);

    //std::cout << "AFTER KD TREE COPY TO BUFFER!" << std::endl;
    ctx.render_context->unmap_buffer(cbounding_box_hierarchy.vertices);

    cbounding_box_hierarchy.indices = ctx.render_device->create_buffer(scm::gl::BIND_INDEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, cbounding_box_hierarchy.indices_count * sizeof(unsigned), kd_tree_.indices.data());

    cbounding_box_hierarchy.vertex_array = ctx.render_device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float)*3) , {cbounding_box_hierarchy.vertices});
    ctx.bounding_box_hierarchies[uuid()] = cbounding_box_hierarchy;

    ctx.render_context->apply();

}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::draw(RenderContext& ctx) const
{
    auto iter = ctx.meshes.find(uuid());
    if(iter == ctx.meshes.end())
    {
        // upload to GPU if neccessary
        upload_to(ctx);
        iter = ctx.meshes.find(uuid());
    }
    ctx.render_context->bind_vertex_array(iter->second.vertex_array);
    ctx.render_context->bind_index_buffer(iter->second.indices, iter->second.indices_topology, iter->second.indices_type);
    ctx.render_context->apply_vertex_input();
    ctx.render_context->draw_elements(iter->second.indices_count);
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::draw_instanced(RenderContext& ctx, int instance_count, int base_vertex, int base_instance) const
{
    auto iter = ctx.meshes.find(uuid());
    if(iter == ctx.meshes.end())
    {
        // upload to GPU if neccessary
        upload_to(ctx);
        iter = ctx.meshes.find(uuid());
    }
    ctx.render_context->bind_vertex_array(iter->second.vertex_array);
    ctx.render_context->bind_index_buffer(iter->second.indices, iter->second.indices_topology, iter->second.indices_type);
    ctx.render_context->apply_vertex_input();
    ctx.render_context->draw_elements_instanced(iter->second.indices_count, 0, instance_count, base_vertex, base_instance);

}

////////////////////////////////////////////////////////////////////////////////
void TriMeshRessource::draw_kdtree(RenderContext& ctx) const {
    auto iter = ctx.bounding_box_hierarchies.find(uuid());
    if(iter == ctx.bounding_box_hierarchies.end())
    {
        // upload to GPU if neccessary
        upload_kdtree_to(ctx);
        iter = ctx.bounding_box_hierarchies.find(uuid());
    }
    ctx.render_context->bind_vertex_array(iter->second.vertex_array);
    ctx.render_context->bind_index_buffer(iter->second.indices, iter->second.indices_topology, iter->second.indices_type);
    ctx.render_context->apply_vertex_input();
    ctx.render_context->draw_elements(iter->second.indices_count);
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::ray_test(Ray const& ray, int options, node::Node* owner, std::set<PickResult>& hits) { kd_tree_.ray_test(ray, mesh_, options, owner, hits); }

////////////////////////////////////////////////////////////////////////////////

math::vec3 TriMeshRessource::get_vertex(unsigned int i) const { return math::vec3(mesh_.positions[i].x, mesh_.positions[i].y, mesh_.positions[i].z); }

////////////////////////////////////////////////////////////////////////////////

std::vector<unsigned int> TriMeshRessource::get_face(unsigned int i) const
{


    std::vector<unsigned int> face(3, 0);

    int64_t const face_base_index = 3 * i;


    face[0] = mesh_.indices[face_base_index + 0];
    face[1] = mesh_.indices[face_base_index + 1];   
    face[2] = mesh_.indices[face_base_index + 2];

    return face;
}

////////////////////////////////////////////////////////////////////////////////


bool TriMeshRessource::save_to_binary(std::string const& input_filename, std::string const& output_filename, const char* filename_gua_kdtree, unsigned flags)
{
    if( !original_material_name_.empty() ) {
        //std::cout << "HAVE FILENAME: " << input_filename << " associated" << std::endl;
        //std::cout << "Going to write file: " << output_filename << std::endl;

        std::string line_buffer;
        std::ifstream in_obj_file(input_filename, std::ios::in);

        std::string out_material_lib_line("");
        bool found_valid_mttllib_token = false;
        while(std::getline(in_obj_file, line_buffer)) {
            if( std::string::npos != line_buffer.find("mtllib ") ) {
                std::istringstream line_buffer_string_stream(line_buffer);
                std::string mtllib_token("");
                line_buffer_string_stream >> mtllib_token;

                if("mtllib" == mtllib_token && !in_obj_file.eof()) {
                    found_valid_mttllib_token = true;
                    std::string mtllib_path("");
                    line_buffer_string_stream >> mtllib_path;
                    out_material_lib_line = "mtllib " + mtllib_path;
                    break;
                }

            }
        }

        if(found_valid_mttllib_token) {
            //std::cout << "Materiallib line: " << line_buffer << std::endl;

            //std::cout << "Writing matref file: " << output_filename << ".mat_ref" << std::endl;
            std::ofstream out_matref_file(output_filename + ".mat_ref", std::ios::out);
            out_matref_file << out_material_lib_line << std::endl;
            out_matref_file << "usemtl " << original_material_name_ << std::endl;
            out_matref_file << "v 0 0 0" << std::endl;
            out_matref_file << "v 1 0 0" << std::endl;            
            out_matref_file << "v 0 1 0" << std::endl;
            out_matref_file << "f 1// 2// 3//" << std::endl;
            out_matref_file.close();
        }

        in_obj_file.close();

    }


    bool res = mesh_.save_to_binary(output_filename, flags);
    if( (nullptr != filename_gua_kdtree) && (0 < kd_tree_.get_num_nodes()) ){
        res = res && kd_tree_.save_to_binary(filename_gua_kdtree);
    }
    return res;
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::set_original_material_name(std::string const& in_material_name) {
    original_material_name_ = in_material_name;
}

////////////////////////////////////////////////////////////////////////////////

std::string TriMeshRessource::get_original_material_name() const {
    return original_material_name_;
}

////////////////////////////////////////////////////////////////////////////////

void TriMeshRessource::set_base_filename(std::string const& in_base_filename) {
    base_filename_ = in_base_filename;
}

////////////////////////////////////////////////////////////////////////////////

std::string TriMeshRessource::get_base_filename() const {
    return base_filename_;
}


} // namespace gua
