module Collada
    class Material
        attr_reader :id
        
        def initialize(id, color)
            @id = id
            @color = color
            end
            
        def Material.from_material(id, material)
            color = material.color.to_a
            color[0] /= 255.0
            color[1] /= 255.0
            color[2] /= 255.0
            color[3] = material.alpha
            self.new(id, color)
            end
            
        def xml_material(parent)
            parent.add_element('material', { 'id' => @id, 'name' => @id }) \
                .add_element('instance_effect', { 'url' => "\##{@id}-effect" })
            end
            
        def xml_effect(parent)
            el_phong = parent.add_element('effect', { 'id' => "#{@id}-effect", 'name' => "#{@id}-effect" }) \
                .add_element('profile_COMMON') \
                .add_element('technique', { 'sid' => "COMMON" }) \
                .add_element('phong')
                
            xml_color(el_phong.add_element('emission'), [0, 0, 0])
            xml_color(el_phong.add_element('ambient'), @color)
            xml_color(el_phong.add_element('diffuse'), @color)
            xml_color(el_phong.add_element('specular'), [0.33, 0.33, 0.33])
            xml_float(el_phong.add_element('shininess'), 20)
            xml_float(el_phong.add_element('reflectivity'), 0.1)
            xml_color(el_phong.add_element('transparent'), [1, 1, 1])
            xml_float(el_phong.add_element('transparency'), 1.0 - @color[3])
            end
            
        def xml_color(parent, color)
            parent.add_element('color') \
                .text = sprintf("%f %f %f 1", color[0], color[1], color[2])
            end
            
        def xml_float(parent, value)
            parent.add_element('float') \
                .text = sprintf("%f", value)
            end
        end
    end
