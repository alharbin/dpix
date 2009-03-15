# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.

module Collada
    class IdManager
        def initialize(initial_entries = [])
            @id_hash = {}
            initial_entries.each { |e| @id_hash[e] = 0 }
            end
        
        def get_id(hint)
            hint.strip!()
            hint_match = hint.match(/(.*[^0-9])[0-9]*/)
            if hint_match
                hint = hint_match[1]
                else
                hint = hint + '_'
                end
            hint.gsub!(/\W/, '_')
            hint.tr!(' ', '_')
            if (@id_hash[hint])
                count = @id_hash[hint] + 1
                @id_hash[hint] = count
                return hint + count.to_s
                else
                @id_hash[hint] = 0
                return hint
                end
            end
        end
    end
