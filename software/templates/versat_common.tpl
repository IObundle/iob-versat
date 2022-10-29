#{define outputName portInstance}
#{set inst2 portInstance.inst}
#{set decl2 inst2.declaration}
   #{if decl2.type == 2}
      in@{inst2.id} #{else}
      #{if decl2.isOperation}
         comb_@{inst2.name |> Identify} #{else}
         output_@{inst2.id}_@{portInstance.port} #{end}
   #{end}
#{end}

#{define CountDones instances}
#{set nDones 0}
#{for inst instances}
#{if inst.declaration.implementsDone}
#{inc nDones}
#{end}
#{end}
#{end}

