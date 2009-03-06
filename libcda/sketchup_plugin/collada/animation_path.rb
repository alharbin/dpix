module Collada
    class AnimationPath
        def AnimationPath.set_path(object)
            object.set_attribute("AnimationPath", "is_path", true)
            end
            
        def AnimationPath.unset_path(object)
            dict = object.attribute_dictionaries['AnimationPath']
            if dict
                dict.delete_key('is_path')
                if dict.length == 0
                    object.attribute_dictionaries.delete(dict)
                    end
                end
            end

        def AnimationPath.is_path?(object)
            object.get_attribute("AnimationPath", "is_path")
            end
            
        def AnimationPath.set_start_edge(object)
            object.set_attribute("AnimationPath", "is_start_edge", true)
            end
            
        def AnimationPath.unset_start_edge(object)
            dict = object.attribute_dictionaries['AnimationPath']
            if dict
                dict.delete_key('is_start_edge')
                if dict.length == 0
                    object.attribute_dictionaries.delete(dict)
                    end
                end
            end

        def AnimationPath.is_start_edge?(object)
            object.get_attribute("AnimationPath", "is_start_edge")
            end
        
        def AnimationPath.convert_to_path(group)
            start_edges = group.entities.select { |edge| AnimationPath.is_start_edge?(edge) }
            if start_edges.empty?
                return nil
                end
                
            vertices = []
                
            current_edge = start_edges[0]
            current_vertex = current_edge.vertices[0]
            vertices << current_vertex
            
            while current_edge do
                current_vertex = current_edge.other_vertex(current_vertex)
                vertices << current_vertex
                current_edges = current_vertex.edges.reject { |edge| edge == current_edge || AnimationPath.is_start_edge?(edge) }
                current_edge = current_edges[0]
                end
            
            vertices.collect { |vertex| group.transformation * vertex.position }
            end
        
        def initialize(mesh)
            @mesh = mesh
            end
           
        def xml_node(parent)
            parent.add_element('instance_path', { 'url' => "\##{@mesh.id}-geometry" })
            end
        end
    end
