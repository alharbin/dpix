require 'rubystdlib.rb'
require 'rexml/Document'

module Collada
    class Mesh
        attr_accessor :id
        
        def initialize
            @vertices = []
            @normals = []
            @geometries = []
            @extra_geometries = []
            @materials = []
            @vertex_hash = {}
            @normal_hash = {}
            end
        
        def transform(transformation, is_flipped)
            matrix = transformation.inverse.to_a
            matrix = [matrix[0, 4], matrix[4, 4], matrix[8, 4], [0, 0, 0, 1]]
            normal_transformation = Geom::Transformation.new(matrix.transpose.flatten)

            @vertices = @vertices.collect { |vertex| transformation * vertex }
            @normals = @normals.collect { |normal| (normal_transformation * normal).normalize }

            if is_flipped
                triangles = @geometries.select { |geometry| geometry.is_a?(Triangles) }
                triangles.each do |triangle|
                    triangle.vertex_indices.reverse!
                    triangle.normal_indices.reverse!
                    end
                end
            end
            
        def add_line_strip(points)
            linestrip = LineStrips.new
            points.each do |point|
                linestrip.vertex_indices << add_vertex(point.to_a)
                end
                
            @geometries << linestrip
            end
        
        def add_faces_edges(faces, edges, material_defs_hash)
            materials_hash = {}
            
            if (faces.length > 0)
                face_normals = calc_normals(faces, edges)
                
                faces.each_index do |fi|
                    material = faces[fi].material
                    if materials_hash[material]
                        materials_hash[material] << fi
                        else
                        materials_hash[material] = [fi]
                        @materials << material_defs_hash[material]
                        end
                    end
                
                materials_hash.each do |material, face_indices|
                    triangles = Triangles.new(material_defs_hash[material])
                    
                    face_indices.each do |fi|
                        mesh = faces[fi].mesh
                        vertex_map = []
                        mesh.points.each do |p|
                            vertex_map << add_vertex(p.to_a)
                            end
                        polygons = mesh.polygons
                        new_vertex_indices = polygons.flatten.collect { |index| vertex_map[index.abs - 1] }
                        triangles.vertex_indices.concat(new_vertex_indices)
                        triangles.count += polygons.length
                        
                        new_normal_indices = []
                        face_points = faces[fi].vertices.collect { |v| v.position.to_a }
                        new_vertex_indices.each do |vi|
                            vertex_num = face_points.index(@vertices[vi])
                            normal = face_normals[fi][vertex_num] if vertex_num
                            if normal
                                new_normal_indices << add_normal(normal.to_a)
                                else
                                new_normal_indices << add_normal(faces[fi].normal.to_a)
                                end
                            end
                        
                        triangles.normal_indices.concat(new_normal_indices)
                        end
                    
                    @geometries << triangles;
                    end
                end

            hard_edges = edges.select { |edge| not edge.soft? and not edge.hidden? }
            if (hard_edges.length > 0)
                lines = Lines.new
                hard_edges.each do |edge|
                    edge.vertices.each do |vertex|
                        lines.vertex_indices << add_vertex(vertex.position.to_a)
                        end
                    end
                lines.count = hard_edges.length
                
                @geometries << lines;
                end

            soft_edges = edges.select { |edge| edge.soft? && edge.faces.length == 2 }
            if (soft_edges.length > 0)
                contours = Contours.new
                soft_edges.each do |edge|
                    edge.vertices.each do |vertex|
                        contours.vertex_indices << add_vertex(vertex.position.to_a)
                        end
                    edge.faces.each do |face|
                        contours.normal_indices << add_normal(face.normal.to_a)
                        end
                    end
                contours.count = soft_edges.length
                
                @extra_geometries << contours;
                end

            end
        
        def calc_normals(faces, edges)
            face_hash = {}
            faces.each { |f| face_hash[f] = face_hash.length }
            smooth_face_lists = faces.collect { [] } # by face
            
            smooth_edges = edges.select { |e| e.smooth? and e.faces.length == 2 }
            smooth_edges.each do |e|
                e.vertices.each do |v|
                    face_index = e.faces.collect { |f| face_hash[f] }
                    vertex_index = e.faces.collect { |f| f.vertices.index(v) }
                    face_lists = []
                    e.faces.each_index { |i| face_lists[i] = smooth_face_lists[face_index[i]][vertex_index[i]] }
                    
                    if face_lists == [nil, nil]
                        # add new list to both
                        new_face_list = [[face_index[0], vertex_index[0]], [face_index[1], vertex_index[1]]]
                        smooth_face_lists[face_index[0]][vertex_index[0]] = new_face_list
                        smooth_face_lists[face_index[1]][vertex_index[1]] = new_face_list
                        elsif face_lists[0] == nil
                        # add 0 to 1's list, copy to 0
                        new_face_list = smooth_face_lists[face_index[1]][vertex_index[1]]
                        #                        new_face_list << face_index[0]
                        new_face_list << [face_index[0], vertex_index[0]]
                        smooth_face_lists[face_index[0]][vertex_index[0]] = new_face_list
                        elsif face_lists[1] == nil
                        # add 1 to 0's list, copy to 1
                        new_face_list = smooth_face_lists[face_index[0]][vertex_index[0]]
                        #                        new_face_list << face_index[1]
                        new_face_list << [face_index[1], vertex_index[1]]
                        smooth_face_lists[face_index[1]][vertex_index[1]] = new_face_list
                        elsif face_lists[0] != face_lists[1]
                        # merge lists
                        # find shortest one to update?
                        # move 1's to 0's, update 1's
                        old_face_list = smooth_face_lists[face_index[1]][vertex_index[1]]
                        new_face_list = smooth_face_lists[face_index[0]][vertex_index[0]]
                        #p "#{([[face_index[0], vertex_index[0]], [face_index[1], vertex_index[1]]]).inspect} merging #{new_face_list.inspect} with #{old_face_list.inspect}"
                        new_face_list.replace(new_face_list | old_face_list)
                        old_face_list.each { |f, vi| smooth_face_lists[f][vi] = new_face_list }
                        end
                    
                    #                    p vertex_index
                    #                    p face_lists
                    end
                end
            
            #            smooth_face_lists.each { |a| p a.collect { |b| b.collect { |c| c[0] } } }
            
            face_normals = faces.collect { [] }
            faces.each_index do |face_index|
                faces[face_index].vertices.each_index do |vertex_index|
                    if smooth_face_lists[face_index][vertex_index]
                        if not face_normals[face_index][vertex_index]
                            normal = Geom::Vector3d.new(0, 0, 0)
                            smooth_face_lists[face_index][vertex_index].each do |fi, vi|
                                normal[0] += faces[fi].normal[0] * faces[fi].area
                                normal[1] += faces[fi].normal[1] * faces[fi].area
                                normal[2] += faces[fi].normal[2] * faces[fi].area
                                end
                            normal.normalize!
                            smooth_face_lists[face_index][vertex_index].each do |fi, vi|
                                face_normals[fi][vi] = normal
                                end
                            end
                        else
                        face_normals[face_index][vertex_index] = faces[face_index].normal
                        end
                    end
                end
            
            face_normals
        end
        
        def add_vertex(vertex)
            add_object(vertex, @vertex_hash, @vertices)
            end
            
        def add_normal(normal)
            add_object(quantize_normal(normal), @normal_hash, @normals)
            end
        
        def add_object(object, hash, array)
            if not hash[object]
                pos = array.length
                hash[object] = pos
                array << object
                return pos
                else
                return hash[object]
                end
            end
            
        def quantize_normal(normal)
            quantized_normal = normal.collect { |v| (v * 1000000).round() * 0.000001 }
            end
        
        def xml_node(parent, default_material)
            el_instance_geometry = parent.add_element('instance_geometry', { 'url' => "\##{@id}-geometry" })
            
            if not @materials.empty?
                el_technique_common = el_instance_geometry.add_element('bind_material') \
                    .add_element('technique_common')
            
                @materials.each do |material|
                    if material == DEFAULT_MATERIAL
                        el_technique_common.add_element('instance_material', { 'symbol' => material.id, 'target' => "\##{default_material.id}" })
                        else
                        el_technique_common.add_element('instance_material', { 'symbol' => material.id, 'target' => "\##{material.id}" })
                        end
                    end
                end
            end
        
        def xml_geometry(parent)
            id = "#{@id}-geometry"
            
            el_mesh = parent.add_element('geometry', { 'id' => id }) \
                .add_element('mesh')
                                
            xml_source(el_mesh, @vertices, "#{id}-position")
            xml_source(el_mesh, @normals, "#{id}-normal")
            
            el_mesh.add_element('vertices', { 'id' => "#{id}-vertex" }) \
                .add_element('input', { 'source' => "\##{id}-position", 'semantic' => 'POSITION' })

            @geometries.each do |geometry|
                geometry.xml(el_mesh, id)
                end
                
            if @extra_geometries.length > 0
                el_dpix = el_mesh.add_element('extra') \
                    .add_element('technique', { 'profile' => 'DPIX' })
                
                @extra_geometries.each do |geometry|
                    geometry.xml(el_dpix, id)
                    end
                end
                
            end
            
        def xml_source(parent, source, id)
            el_source = parent.add_element('source', { 'id' => id })

            el_source.add_element('float_array', { 'id' => "#{id}-array", 'count' => source.length * 3 }) \
                .text = (source.collect { |p| Collada.p_to_s(p) }) * ' '
            
            el_accessor = el_source.add_element('technique_common') \
                .add_element('accessor', { 'source' => "\##{id}-array", 'count' => source.length, 'stride' => 3 })
                
            el_accessor.add_element('param', { 'name' => 'X', 'type' => 'float' })
            el_accessor.add_element('param', { 'name' => 'Y', 'type' => 'float' })
            el_accessor.add_element('param', { 'name' => 'Z', 'type' => 'float' })
            end
        end
    end
