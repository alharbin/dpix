# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.

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
