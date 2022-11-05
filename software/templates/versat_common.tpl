#{define outputName portInstance}
#{set inst2 portInstance.inst}
#{set decl2 inst2.declaration}
   #{if decl2.type == 2}
      in@{inst2.id} 
   #{else}
      #{if decl2.isOperation}
         #{if decl2.latencies[0] == 0}
            comb_@{inst2.name |> Identify}
         #{else}
            seq_@{inst2.name |> Identify}
         #{end}
      #{else}
         output_@{inst2.id}_@{portInstance.port} 
      #{end}
   #{end}
#{end}

#{define retOutputName portInstance}
#{set inst2 portInstance.inst}
#{set decl2 inst2.declaration}
   #{if decl2.type == 2}
      #{return "in" # inst2.id} 
   #{else}
      #{if decl2.isOperation}
         #{return "comb_" # inst2.name |> Identify} 
      #{else}
         #{return "output_" # inst2.id # "_" # portInstance.port} 
      #{end}
   #{end}
#{end}

#{define CountDones instances}
   #{set nDones 0}
   #{for inst instances}
   #{if inst.declaration.implementsDone}
   #{inc nDones}
   #{end}
   #{end}
   #{return nDones}
#{end}

#{define CountOperations instances}
   #{set nOperations 0}
   #{set nCombOperations 0}
   #{set nSeqOperations 0}
   #{for inst instances}
      #{if inst.declaration.isOperation}
         #{inc nOperations}
         #{if inst.declaration.latencies[0] == 0}
            #{inc nCombOperations}
         #{else}
            #{inc nSeqOperations}
         #{end}
      #{end}
   #{end}
   #{return nOperations}
#{end}

