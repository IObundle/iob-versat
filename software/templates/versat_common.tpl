#{define retIterativeOutputName portInstance port}
#{set res ""}
#{if portInstance.inst.declaration == versat.data}
   #{set res "data[" # port # "]"}
#{else}
#{if portInstance.inst.declaration.type == 2}
   #{set res "in" # port}
#{else}
   #{set res "unitOut[" # port # "]"}
#{end}
#{end}
#{return res}
#{end}

#{define iterativeOutputName portInstance port}
#{set val #{call retIterativeOutputName portInstance port}}
@{val}#{end}

#{define retOutputName2 instances portNode}
#{let id 0}
#{for node instances}
  #{if node.inst == portNode.node.inst}
    #{set id index}
  #{end}
#{end}
#{let res ""}
#{if portNode.node}
#{let inst2 portNode.node.inst}
#{let decl2 inst2.declaration}
   #{if decl2.type == 2}
      #{set res "in" # inst2.portIndex}
   #{else}
      #{if decl2.isOperation}
         #{if decl2.baseConfig.outputLatencies[0] == 0}
            #{set res "comb_" # inst2.name |> Identify}
         #{else}
            #{set res "seq_" # inst2.name |> Identify}
         #{end} 
      #{else}
         #{set res "output_" # @{id} # "_" # portNode.port} 
      #{end}
   #{end}
#{else}
#{let res "0"}
#{end}
#{return res}
#{end}

#{define retOutputName portNode}
#{set res ""}
#{if portNode.node}
#{set inst2 portNode.node.inst}
#{set decl2 inst2.declaration}
   #{if decl2.type == 2}
      #{set res "in" # inst2.portIndex}
   #{else}
      #{if decl2.isOperation}
         #{if decl2.baseConfig.outputLatencies[0] == 0}
            #{set res "comb_" # inst2.name |> Identify}
         #{else}
            #{set res "seq_" # inst2.name |> Identify}
         #{end} 
      #{else}
         #{set res "output_" # inst2.id # "_" # portNode.port} 
      #{end}
   #{end}
#{else}
#{set res "0"}
#{end}
#{return res}
#{end}

#{define outputName portInstance}
#{set val #{call retOutputName portInstance}} @{val} #{end}

#{define CountDones instances}
   #{set nDones 0}
   #{for node instances}
   #{if node.inst.declaration.implementsDone}
   #{inc nDones}
   #{end}
   #{end}
   #{return nDones}
#{end}

#{define CountOperations instances}
   #{set nOperations 0}
   #{for node instances}
      #{if node.inst.declaration.isOperation}
         #{inc nOperations}
      #{end}
   #{end}
   #{return nOperations}
#{end}

#{define CountCombOperations instances}
   #{set nCombOperations 0}
   #{for node instances}
      #{if node.inst.declaration.isOperation}
         #{if node.inst.declaration.baseConfig.outputLatencies[0] == 0}
            #{inc nCombOperations}
         #{end}
      #{end}
   #{end}
   #{return nCombOperations}
#{end}

#{define CountSeqOperations instances}
   #{set nSeqOperations 0}
   #{for node instances}
      #{if node.inst.declaration.isOperation}
         #{if node.inst.declaration.baseConfig.outputLatencies[0] != 0}
            #{inc nSeqOperations}
         #{end}
      #{end}
   #{end}
   #{return nSeqOperations}
#{end}

#{define IntName count}
#{set name "int"}
#{if count == 8}
#{set name "int8"}
#{end}
#{if count == 16}
#{set name "int16"}
#{end}
#{if count == 64}
#{set name "int64"}
#{end}
#{if count == 128}
#{set name "Int128"}
#{end}
#{if count == 256}
#{set name "Int256"}
#{end}
#{return name}
#{end}
