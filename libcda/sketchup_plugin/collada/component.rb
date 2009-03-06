module Collada
    class Component
        attr_accessor :node
        def initialize(node)
            @node = node
            end
            
        def xml_node(parent)
            parent.add_element('instance_node', { 'url' => "\##{node.id}" })
            end
        end
    end
