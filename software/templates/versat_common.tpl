#{define outputName portInstance}
#{set inst portInstance.inst}
#{set decl inst.declaration}
   #{if decl.type == 2}
      in@{inst.id} #{else}
      #{if decl.isOperation}
         comb_@{inst.name.str} #{else}
         output_@{inst.id}_@{portInstance.port} #{end}
   #{end}
#{end}


