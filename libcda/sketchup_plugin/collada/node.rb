module Collada
    class Node
        attr_accessor :name, :id, :geometries, :components, :nodes, :animation_path, :transformation, :material
        
        def initialize
            @geometries = []
            @components = []
            @nodes = []
            end
            
        def xml_node(parent, default_material)
            el_node = parent.add_element('node')
            el_node.add_attribute('id', @id) if @id
            el_node.add_attribute('name', @name) if @name
            
            if @transformation then
                matrix = @transformation.to_a
                matrix = matrix.collect { |n| if n.abs < 1e-10 then 0.0 else n end }
                matrix = [matrix[0, 4], matrix[4, 4], matrix[8, 4], matrix[12, 4]]
                matrix = matrix.transpose
                el_node.add_element('matrix') \
                    .text = matrix * ' '
                end
            
            default_material = @material if @material
            @geometries.each do |geometry|
                geometry.xml_node(el_node, default_material)
                end
            @components.each do |component|
                component.xml_node(el_node)
                end
            @nodes.each do |child|
                child.xml_node(el_node, default_material)
                end
                
            if animation_path
                el_dpix = el_node.add_element('extra') \
                    .add_element('technique', { 'profile' => 'DPIX' })

                animation_path.xml_node(el_dpix)
                end
            end
        end
    end
