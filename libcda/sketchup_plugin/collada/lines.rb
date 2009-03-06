module Collada
    class Lines
        attr_accessor :vertex_indices, :count
        
        def initialize
            @vertex_indices = []
            @count = 0
            end
            
        def xml(parent, id)
            el_lines = parent.add_element('lines', { 'count' => @count })
                    
            el_lines.add_element('input', { 'semantic' => 'VERTEX', 'source' => "\##{id}-vertex", 'offset' => 0 })
            
            el_lines.add_element('p') \
                .text = @vertex_indices * ' '
            end
        end
    end
