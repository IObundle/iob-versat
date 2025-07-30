#include "codeGeneration.hpp"

#include "CEmitter.hpp"
#include "VerilogEmitter.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"
#include "declaration.hpp"
#include "embeddedData.hpp"
#include "filesystem.hpp"
#include "memory.hpp"
#include "symbolic.hpp"
#include "addressGen.hpp"
#include "globals.hpp"
#include "templateEngine.hpp"

#include "utilsCore.hpp"
#include "versatSpecificationParser.hpp"

// TODO: If we could find a way of describing the format of verilog modules that we care about, we could also perform
//       a check at Versat runtime that we are producing correct code, although it might take longer than the simple
//       loop of making a change and checking if Verilator/Icarus complains about it.

// TODO: Is there a way of describing the format of the wires for an interface and have the code generate it automatically? We would need to be able to describe the format of the wires, the place where the indexes change, the place where the port index changes, etc. So far, this would only be needed if we added more memory types or if we started generating more code.

// TODO: Emit functions do not need to return a String. They could just write directly the result to a file and skip having to push a new string for no reason.

// TODO: Move all this stuff to a better place
#include <filesystem>
namespace fs = std::filesystem;

String GetRelativePathFromSourceToTarget(String sourcePath,String targetPath,Arena* out){
  TEMP_REGION(temp,out);
  fs::path source(StaticFormat("%.*s",UNPACK_SS(sourcePath)));
  fs::path target(StaticFormat("%.*s",UNPACK_SS(targetPath)));

  fs::path res = fs::relative(target,source);

  return PushString(out,"%s",res.c_str());
}

Array<String> ExtractMemoryMasks(AccelInfo info,Arena* out){
  auto builder = StartArray<String>(out);
  for(AccelInfoIterator iter = StartIteration(&info); iter.IsValid(); iter = iter.Next()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->memDecisionMask.has_value()){
      *builder.PushElem() = unit->memDecisionMask.value();
    } else if(unit->memMapped.has_value()){
      *builder.PushElem() = {}; // Empty mask (happens when only one unit with mem exists, bit weird but no need to change for now);
    }
  }

  return EndArray(builder);
}

Array<Array<MuxInfo>> CalculateMuxInformation(AccelInfoIterator* iter,Arena* out){
  auto ExtractMuxInfo = [](AccelInfoIterator iter,Arena* out) -> Array<MuxInfo>{
    TEMP_REGION(temp,out);

    auto alreadySet = StartArray<bool>(temp);
    
    auto builder = StartArray<MuxInfo>(out);
    for(; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->isMergeMultiplexer){
        int muxGroup = info->muxGroup;
        if(!alreadySet[muxGroup]){
          builder[muxGroup].configIndex = info->globalConfigPos.value();
          builder[muxGroup].val = info->mergePort;
          builder[muxGroup].name = info->baseName;
          builder[muxGroup].fullName = info->fullName;
          alreadySet[muxGroup] = true;
        }
      }
    }

    return EndArray(builder);
  };

  Array<Array<MuxInfo>> muxInfo = PushArray<Array<MuxInfo>>(out,iter->MergeSize());
  for(int i = 0; i < iter->MergeSize(); i++){
    AccelInfoIterator it = *iter;
    it.SetMergeIndex(i);
    muxInfo[i] = ExtractMuxInfo(it,out);
  }

  bool allEmpty = true;
  for(Array<MuxInfo> muxes : muxInfo){
    if(muxes.size > 0){
      allEmpty = false;
    }
  }

  if(allEmpty){
    return {};
  }
  
  return muxInfo;
}

Array<Array<InstanceInfo*>> VUnitInfoPerMerge(AccelInfo info,Arena* out){
  TEMP_REGION(temp,out);

  auto IsVUnit = [](FUDeclaration* decl){
    // TODO: We need to do this properly if we want to push more runtime stuff related to VUnits.
    return decl->supportedAddressGenType == AddressGenType_READ;
  };

  AccelInfoIterator iter = StartIteration(&info);
  Array<Array<InstanceInfo*>> unitInfoPerMerge = PushArray<Array<InstanceInfo*>>(out,iter.MergeSize());
  for(int i = 0; i < iter.MergeSize(); i++){
    AccelInfoIterator it = iter;
    it.SetMergeIndex(i);

    auto list = PushArenaList<InstanceInfo*>(temp);
    for(; it.IsValid(); it = it.Step()){
      InstanceInfo* info = it.CurrentUnit();
      if(info->doesNotBelong){
        continue;
      }

      if(!IsVUnit(info->decl)){
        continue;
      }
        
      if(info->addressGenUsed.size < 1){
        continue;
      }
        
      *list->PushElem() = info;
    }

    unitInfoPerMerge[i] = PushArrayFromList(temp,list);
  }

  return unitInfoPerMerge;
}

String GenerateVerilogParameterization(FUInstance* inst,Arena* out){
  TEMP_REGION(temp,out);
  FUDeclaration* decl = inst->declaration;
  Array<Parameter> parameters = decl->parameters;
  int size = parameters.size;

  if(size == 0){
    return {};
  }
  
  auto mark = MarkArena(out);
  
  auto builder = StartString(temp);
  builder->PushString("#(");
  bool insertedOnce = false;
  for(int i = 0; i < size; i++){
    ParameterValue v = inst->parameterValues[i];
    Parameter parameter = parameters[i];
    
    if(!v.val.size){
      continue;
    }

    if(insertedOnce){
      builder->PushString(",");
    }
    builder->PushString(".%.*s(%.*s)",UNPACK_SS(parameter.name),UNPACK_SS(v.val));
    insertedOnce = true;
  }
  builder->PushString(")");

  if(!insertedOnce){
    PopMark(mark);
    return {};
  }

  return EndString(out,builder);
}

String EmitExternalMemoryInstances(Array<ExternalMemoryInterface> external,Arena* out){
  TEMP_REGION(temp,out);

  VEmitter* m = StartVCode(temp);

  for(int i = 0; i < external.size; i++){
    ExternalMemoryInterface ext = external[i];

    FULL_SWITCH(ext.type){
    case ExternalMemoryType::DP:{
      m->StartInstance("my_dp_asym_ram",SF("ext_dp_%d",i));
      {
        m->InstanceParam("A_DATA_W",ext.dp[0].dataSizeOut);
        m->InstanceParam("B_DATA_W",ext.dp[1].dataSizeOut);
        m->InstanceParam("ADDR_W",ext.dp[0].bitSize);

        m->PortConnect("dinA_i",SF("VERSAT0_ext_dp_out_%d_port_0_o",i));
        m->PortConnect("addrA_i",SF("VERSAT0_ext_dp_addr_%d_port_0_o",i));
        m->PortConnect("enA_i",SF("VERSAT0_ext_dp_enable_%d_port_0_o",i));
        m->PortConnect("weA_i",SF("VERSAT0_ext_dp_write_%d_port_0_o",i));
        m->PortConnect("doutA_o",SF("VERSAT0_ext_dp_in_%d_port_0_i",i));

        m->PortConnect("dinB_i",SF("VERSAT0_ext_dp_out_%d_port_1_o",i));
        m->PortConnect("addrB_i",SF("VERSAT0_ext_dp_addr_%d_port_1_o",i));
        m->PortConnect("enB_i",SF("VERSAT0_ext_dp_enable_%d_port_1_o",i));
        m->PortConnect("weB_i",SF("VERSAT0_ext_dp_write_%d_port_1_o",i));
        m->PortConnect("doutB_o",SF("VERSAT0_ext_dp_in_%d_port_1_i",i));

        m->PortConnect("clk_i","clk_i");
      }      
      m->EndInstance();
    } break;
    case ExternalMemoryType::TWO_P:{
      m->StartInstance("my_2p_asym_ram",SF("ext_2p_%d",i));
      {
        m->InstanceParam("W_DATA_W",ext.tp.dataSizeOut);
        m->InstanceParam("R_DATA_W",ext.tp.dataSizeIn);
        m->InstanceParam("ADDR_W",ext.tp.bitSizeIn);

        m->PortConnect("w_en_i",SF("VERSAT0_ext_2p_write_%d_o",i));
        m->PortConnect("w_addr_i",SF("VERSAT0_ext_2p_addr_out_%d_o",i));
        m->PortConnect("w_data_i",SF("VERSAT0_ext_2p_data_out_%d_o",i));

        m->PortConnect("r_en_i",SF("VERSAT0_ext_2p_read_%d_o",i));
        m->PortConnect("r_addr_i",SF("VERSAT0_ext_2p_addr_in_%d_o",i));
        m->PortConnect("r_data_o",SF("VERSAT0_ext_2p_data_in_%d_i",i));

        m->PortConnect("clk_i","clk_i");
      }
      m->EndInstance();
    }
  } END_SWITCH();
  }

  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(out,b);
  return content;
}

String EmitExternalMemoryPort(Array<ExternalMemoryInterface> external,Arena* out){
  TEMP_REGION(temp,out);

  VEmitter* m = StartVCode(temp);

  m->StartPortGroup();
  
  for(int i = 0; i < external.size; i++){
    ExternalMemoryInterface ext = external[i];

    FULL_SWITCH(ext.type){
    case ExternalMemoryType::DP:{
      for(int k = 0; k < 2; k++){
        m->Output(SF("ext_dp_addr_%d_port_%d_o",i,k),ext.dp[k].bitSize);
        m->Output(SF("ext_dp_out_%d_port_%d_o",i,k),ext.dp[k].dataSizeOut);
        m->Input(SF("ext_dp_in_%d_port_%d_i",i,k),ext.dp[k].dataSizeIn);
        m->Output(SF("ext_dp_enable_%d_port_%d_o",i,k));
        m->Output(SF("ext_dp_write_%d_port_%d_o",i,k));
      }      
    } break;
    case ExternalMemoryType::TWO_P:{
      m->Output(SF("ext_2p_write_%d_o",i));
      m->Output(SF("ext_2p_addr_out_%d_o",i),ext.tp.bitSizeOut);
      m->Output(SF("ext_2p_data_out_%d_o",i),ext.tp.dataSizeOut);

      m->Output(SF("ext_2p_read_%d_o",i));
      m->Output(SF("ext_2p_addr_in_%d_o",i),ext.tp.bitSizeIn);
      m->Input(SF("ext_2p_data_in_%d_i",i),ext.tp.dataSizeIn);
    } break;
  } END_SWITCH()
  }

  m->EndPortGroup();

  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(out,b);

  return content;
}

String EmitExternalMemoryInternalPortmap(Array<ExternalMemoryInterface> external,Arena* out){
  TEMP_REGION(temp,out);

  VEmitter* m = StartVCode(temp);

  for(int i = 0; i < external.size; i++){
    ExternalMemoryInterface ext = external[i];

    FULL_SWITCH(ext.type){
    case ExternalMemoryType::DP:{
      for(int k = 0; k < 2; k++){
        m->PortConnect(PushString(temp,"ext_dp_out_%d_port_%d_o",i,k),PushString(temp,"ext_dp_out_%d_port_%d_o",i,k));
        m->PortConnect(PushString(temp,"ext_dp_addr_%d_port_%d_o",i,k),PushString(temp,"ext_dp_addr_%d_port_%d_o",i,k));
        m->PortConnect(PushString(temp,"ext_dp_enable_%d_port_%d_o",i,k),PushString(temp,"ext_dp_enable_%d_port_%d_o",i,k));
        m->PortConnect(PushString(temp,"ext_dp_write_%d_port_%d_o",i,k),PushString(temp,"ext_dp_write_%d_port_%d_o",i,k));
        m->PortConnect(PushString(temp,"ext_dp_in_%d_port_%d_i",i,k),PushString(temp,"ext_dp_in_%d_port_%d_i",i,k));
      }
    } break;
    case ExternalMemoryType::TWO_P:{
      m->PortConnect(PushString(temp,"ext_2p_write_%d_o",i),PushString(temp,"ext_2p_write_%d_o",i));
      m->PortConnect(PushString(temp,"ext_2p_addr_out_%d_o",i),PushString(temp,"ext_2p_addr_out_%d_o",i));
      m->PortConnect(PushString(temp,"ext_2p_data_out_%d_o",i),PushString(temp,"ext_2p_data_out_%d_o",i));

      m->PortConnect(PushString(temp,"ext_2p_read_%d_o",i),PushString(temp,"ext_2p_read_%d_o",i));
      m->PortConnect(PushString(temp,"ext_2p_addr_in_%d_o",i),PushString(temp,"ext_2p_addr_in_%d_o",i));
      m->PortConnect(PushString(temp,"ext_2p_data_in_%d_i",i),PushString(temp,"ext_2p_data_in_%d_i",i));
    }
  } END_SWITCH();
  }

  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(out,b);
  return content;
}

String EmitInternalMemoryWires(Array<ExternalMemoryInterface> external,Arena* out){
  TEMP_REGION(temp,out);

  VEmitter* m = StartVCode(temp);

  for(int i = 0; i < external.size; i++){
    ExternalMemoryInterface ext = external[i];

    FULL_SWITCH(ext.type){
    case ExternalMemoryType::DP:{
      for(int k = 0; k < 2; k++){
        m->Wire(SF("ext_dp_addr_%d_port_%d_o",i,k),ext.dp[k].bitSize);
        m->Wire(SF("ext_dp_out_%d_port_%d_o",i,k),ext.dp[k].dataSizeOut);
        m->Wire(SF("ext_dp_in_%d_port_%d_i",i,k),ext.dp[k].dataSizeIn);
        m->Wire(SF("ext_dp_enable_%d_port_%d_o",i,k));
        m->Wire(SF("ext_dp_write_%d_port_%d_o",i,k));

        m->Wire(SF("VERSAT0_ext_dp_addr_%d_port_%d_o",i,k),ext.dp[k].bitSize);
        m->Wire(SF("VERSAT0_ext_dp_out_%d_port_%d_o",i,k),ext.dp[k].dataSizeOut);
        m->Wire(SF("VERSAT0_ext_dp_in_%d_port_%d_i",i,k),ext.dp[k].dataSizeIn);
        m->Wire(SF("VERSAT0_ext_dp_enable_%d_port_%d_o",i,k));
        m->Wire(SF("VERSAT0_ext_dp_write_%d_port_%d_o",i,k));

        m->Assign(PushString(temp,"VERSAT0_ext_dp_addr_%d_port_%d_o",i,k),PushString(temp,"ext_dp_addr_%d_port_%d_o",i,k));
        m->Assign(PushString(temp,"VERSAT0_ext_dp_out_%d_port_%d_o",i,k),PushString(temp,"ext_dp_out_%d_port_%d_o",i,k));
        m->Assign(PushString(temp,"ext_dp_in_%d_port_%d_i",i,k),PushString(temp,"VERSAT0_ext_dp_in_%d_port_%d_i",i,k));
        m->Assign(PushString(temp,"VERSAT0_ext_dp_enable_%d_port_%d_o",i,k),PushString(temp,"ext_dp_enable_%d_port_%d_o",i,k));
        m->Assign(PushString(temp,"VERSAT0_ext_dp_write_%d_port_%d_o",i,k),PushString(temp,"ext_dp_write_%d_port_%d_o",i,k));
      }
    } break;
    case ExternalMemoryType::TWO_P:{
      m->Wire(SF("ext_2p_addr_out_%d_o",i),ext.tp.bitSizeOut);
      m->Wire(SF("ext_2p_addr_in_%d_o",i),ext.tp.bitSizeIn);
      m->Wire(SF("ext_2p_data_in_%d_i",i),ext.tp.dataSizeIn);
      m->Wire(SF("ext_2p_data_out_%d_o",i),ext.tp.dataSizeOut);
      m->Wire(SF("ext_2p_write_%d_o",i));
      m->Wire(SF("ext_2p_read_%d_o",i));

      m->Wire(SF("VERSAT0_ext_2p_addr_out_%d_o",i),ext.tp.bitSizeOut);
      m->Wire(SF("VERSAT0_ext_2p_addr_in_%d_o",i),ext.tp.bitSizeIn);
      m->Wire(SF("VERSAT0_ext_2p_data_in_%d_i",i),ext.tp.dataSizeIn);
      m->Wire(SF("VERSAT0_ext_2p_data_out_%d_o",i),ext.tp.dataSizeOut);
      m->Wire(SF("VERSAT0_ext_2p_write_%d_o",i));
      m->Wire(SF("VERSAT0_ext_2p_read_%d_o",i));

      m->Assign(PushString(temp,"VERSAT0_ext_2p_addr_out_%d_o",i),PushString(temp,"ext_2p_addr_out_%d_o",i));
      m->Assign(PushString(temp,"VERSAT0_ext_2p_addr_in_%d_o",i),PushString(temp,"ext_2p_addr_in_%d_o",i));
      m->Assign(PushString(temp,"ext_2p_data_in_%d_i",i),PushString(temp,"VERSAT0_ext_2p_data_in_%d_i",i));
      m->Assign(PushString(temp,"VERSAT0_ext_2p_data_out_%d_o",i),PushString(temp,"ext_2p_data_out_%d_o",i));
      m->Assign(PushString(temp,"VERSAT0_ext_2p_write_%d_o",i),PushString(temp,"ext_2p_write_%d_o",i));
      m->Assign(PushString(temp,"VERSAT0_ext_2p_read_%d_o",i),PushString(temp,"ext_2p_read_%d_o",i));
    } break;
    } END_SWITCH();
  }

  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(out,b);
  return content;
}

static String OutputName(Pool<FUInstance>& instances,PortInstance node,Arena* out){
  FUInstance* inst = node.inst;
  FUDeclaration* decl = inst->declaration;

  int id = 0;
  for(FUInstance* ptr : instances){
    if(ptr == inst){
      break;
    }
    id++;
  }
    
  if(decl == BasicDeclaration::input){
    return PushString(out,"in%d",inst->portIndex);
  }
  if(decl->IsCombinatorialOperation()){
    return PushString(out,"comb_%.*s_%d",UNPACK_SS(inst->name),inst->id);
  }
  if(decl->IsSequentialOperation()){
    return PushString(out,"seq_%.*s_%d",UNPACK_SS(inst->name),inst->id);
  }
    
  return PushString(out,"output_%d_%d",id,node.port);
};

void EmitCombOperations(VEmitter* m,Pool<FUInstance> instances){
  TEMP_REGION(temp,m->arena);

  auto Format = [](const char* format,Array<String> args,Arena* out){
    TEMP_REGION(temp,out);
    StringBuilder* b = StartString(temp);
    
    Tokenizer tok(STRING(format),"{}",{});
    while(!tok.Done()){
      Opt<Token> simpleText = tok.PeekFindUntil("{");

      if(!simpleText.has_value()){
        simpleText = tok.Finish();
      }

      tok.AdvancePeekBad(simpleText.value());

      b->PushString(simpleText.value());

      if(tok.Done()){
        break;
      }

      tok.AssertNextToken("{");
      Token indexText = tok.NextToken();
      tok.AssertNextToken("}");

      int index = ParseInt(indexText);
      
      Assert(index < args.size);
      
      b->PushString(args[index]);
    }

    return EndString(out,b);
  };
  
  int combOps = 0;
  for(FUInstance* node : instances){
    if(node->declaration->IsCombinatorialOperation()){
      combOps += 1;
    }
  }
  
  if(combOps){
    for(FUInstance* node : instances){
      if(node->declaration->IsCombinatorialOperation()){
        m->Reg(SF("comb_%.*s_%d",UNPACK_SS(node->name),node->id),S8("DATA_W"));
      }
    }

    m->CombBlock();
    {
      for(FUInstance* node : instances){
        if(node->declaration->IsCombinatorialOperation()){
          int size = node->declaration->NumberInputs();
          String identifier = PushString(temp,"comb_%.*s_%d",UNPACK_SS(node->name),node->id);
            
          Array<String> args = PushArray<String>(temp,size);
          for(int i = 0; i < size; i++){
            args[i] = OutputName(instances,node->inputs[i],temp);
          }
          String expr = Format(node->declaration->operation,args,temp);
          m->Set(identifier,expr);
        }
      }
    }
    m->EndBlock();
  }
}

// TODO: We want to merge this with the TopLevelInstanciateUnits. 
void EmitInstanciateUnits(VEmitter* m,Pool<FUInstance> instances,FUDeclaration* module,Array<Array<int>> wireIndexByInstanceGood,Array<Wire> configs){
  TEMP_REGION(temp,m->arena);
  
  int delaySeen = 0;
  int statesSeen = 0;
  int doneSeen = 0;
  int ioSeen = 0;
  int memoryMappedSeen = 0;
  int externalSeen = 0;
    
  for(int instIndex = 0; instIndex < instances.Size(); instIndex++){
    FUInstance* inst = instances.Get(instIndex);
    FUDeclaration* decl = inst->declaration;
    if(decl == BasicDeclaration::input || decl == BasicDeclaration::output || decl->IsCombinatorialOperation()){
      continue;
    }

    m->StartInstance(decl->name,SF("%.*s_%d",UNPACK_SS(inst->name),instIndex));

    Hashmap<String,SymbolicExpression*>* params = GetParametersOfUnit(inst,temp);
    for(Pair<String,SymbolicExpression**> p : params){
      if(p.second){
        String repr = PushRepresentation(*p.second,temp);
        m->InstanceParam(CS(p.first),CS(repr));
      }
    }
    if(decl->memoryMapBits.has_value()){
      SymbolicExpression* expr = PushLiteral(temp,decl->memoryMapBits.value());
      params->Insert(S8("ADDR_W"),expr);
    }
    
    for(int i = 0; i < inst->outputs.size; i++){
      if(inst->outputs[i]){
        m->PortConnectIndexed("out%d",i,SF("output_%d_%d",instIndex,i));
      } else {
        m->PortConnectIndexed("out%d",i,"");
      }
    }

    for(int i = 0; i < inst->inputs.size; i++){
      if(inst->inputs[i].inst){
        PortInstance other = GetAssociatedOutputPortInstance(inst,i);
        m->PortConnectIndexed("in%d",i,OutputName(instances,other,temp));
      } else {
        m->PortConnectIndexed("in%d",i,"0");
      }
    }

    // Configs and dealing with static configs if the unit is static
    if(inst->isStatic){
      for(Wire w : decl->configs){
        m->PortConnect(w.name,PushString(temp,"%.*s_%.*s_%.*s",UNPACK_SS(module->name),UNPACK_SS(inst->name),UNPACK_SS(w.name)));
      }
    } else {
      Array<int> ind = wireIndexByInstanceGood[instIndex];
      for(int i = 0; i < ind.size; i++){
        m->PortConnect(PushString(temp,"%.*s",UNPACK_SS(decl->configs[i].name)),configs[ind[i]].name);
      }
    }

    // Static configs
    for(Pair<StaticId,StaticData*> p : decl->staticUnits){
      StaticId id = p.first;
      for(Wire w : p.second->configs){
        String wireStaticName = PushString(temp,"%.*s_%.*s_%.*s",UNPACK_SS(id.parent->name),UNPACK_SS(id.name),UNPACK_SS(w.name));
          
        m->PortConnect(wireStaticName,wireStaticName);
      }
    }

    // State
    for(Wire w : decl->states){
      m->PortConnect(w.name,module->states[statesSeen++].name);
    }
      
    // Delays
    for(int i = 0; i < decl->numberDelays; i++){
      m->PortConnectIndexed("delay%d",i,SF("delay%d",delaySeen++));
    }

    // External memories
    for(int i = 0; i <  decl->externalMemory.size; i++){
      ExternalMemoryInterface ext = decl->externalMemory[i];
      if(ext.type == ExternalMemoryType::DP){
        m->PortConnectIndexed("ext_dp_addr_%d_port_0",i,"ext_dp_addr_%d_port_0",externalSeen);
        m->PortConnectIndexed("ext_dp_out_%d_port_0",i,"ext_dp_out_%d_port_0",externalSeen);
        m->PortConnectIndexed("ext_dp_in_%d_port_0",i,"ext_dp_in_%d_port_0",externalSeen);
        m->PortConnectIndexed("ext_dp_enable_%d_port_0",i,"ext_dp_enable_%d_port_0",externalSeen);
        m->PortConnectIndexed("ext_dp_write_%d_port_0",i,"ext_dp_write_%d_port_0",externalSeen);

        m->PortConnectIndexed("ext_dp_addr_%d_port_1",i,"ext_dp_addr_%d_port_1",externalSeen);
        m->PortConnectIndexed("ext_dp_out_%d_port_1",i,"ext_dp_out_%d_port_1",externalSeen);
        m->PortConnectIndexed("ext_dp_in_%d_port_1",i,"ext_dp_in_%d_port_1",externalSeen);
        m->PortConnectIndexed("ext_dp_enable_%d_port_1",i,"ext_dp_enable_%d_port_1",externalSeen);
        m->PortConnectIndexed("ext_dp_write_%d_port_1",i,"ext_dp_write_%d_port_1",externalSeen);
      } else {
        m->PortConnectIndexed("ext_2p_addr_out_%d",i,"ext_2p_addr_out_%d",externalSeen);
        m->PortConnectIndexed("ext_2p_addr_in_%d",i,"ext_2p_addr_in_%d",externalSeen);
        m->PortConnectIndexed("ext_2p_write_%d",i,"ext_2p_write_%d",externalSeen);
        m->PortConnectIndexed("ext_2p_read_%d",i,"ext_2p_read_%d",externalSeen);
        m->PortConnectIndexed("ext_2p_data_in_%d",i,"ext_2p_data_in_%d",externalSeen);
        m->PortConnectIndexed("ext_2p_data_out_%d",i,"ext_2p_data_out_%d",externalSeen);
      }
      externalSeen += 1;
    }

    // Memory mapping
    if(decl->memoryMapBits.has_value()){
      m->PortConnect("valid",SF("memoryMappedEnable[%d]",memoryMappedSeen));
      m->PortConnect("wstrb","wstrb");
      if(decl->memoryMapBits.value() > 0){
        m->PortConnect("addr",SF("addr[%d-1:0]",decl->memoryMapBits.value()));
      }
      m->PortConnect("rdata",SF("unitRData[%d]",memoryMappedSeen));
      m->PortConnect("rvalid",SF("unitRValid[%d]",memoryMappedSeen));
      m->PortConnect("wdata","wdata");
        
      memoryMappedSeen += 1;
    }

    // Databus
    for(int i = 0; i < decl->nIOs; i++){
      m->PortConnectIndexed("databus_ready_%d",i,"databus_ready_%d",ioSeen);
      m->PortConnectIndexed("databus_valid_%d",i,"databus_valid_%d",ioSeen);
      m->PortConnectIndexed("databus_addr_%d",i,"databus_addr_%d",ioSeen);
      m->PortConnectIndexed("databus_rdata_%d",i,"databus_rdata_%d",ioSeen);
      m->PortConnectIndexed("databus_wdata_%d",i,"databus_wdata_%d",ioSeen);
      m->PortConnectIndexed("databus_wstrb_%d",i,"databus_wstrb_%d",ioSeen);
      m->PortConnectIndexed("databus_len_%d",i,"databus_len_%d",ioSeen);
      m->PortConnectIndexed("databus_last_%d",i,"databus_last_%d",ioSeen);

      ioSeen += 1;
    }

    if(decl->signalLoop){
      m->PortConnect("signal_loop","signal_loop");
    }

    m->PortConnect("running","running");
    m->PortConnect("run","run");

    if(decl->implementsDone){
      m->PortConnect("done",SF("unitDone[%d]",doneSeen++));
    }

    m->PortConnect("clk","clk");
    m->PortConnect("rst","rst");
      
    m->EndInstance();
  }
}

// TODO: We want to merge this function with EmitInstanceateUnits. They are basically the same code but need to handle the top level different in regards to the way wires are connection in relation to configs, states and stuff like that.
void EmitTopLevelInstanciateUnits(VEmitter* m,VersatComputedValues val,Pool<FUInstance> instances,FUDeclaration* module,Array<Wire> configs){
  TEMP_REGION(temp,m->arena);
  
  int doneSeen = 0;
  int ioSeen = 0;
  int memoryMappedSeen = 0;
  int externalSeen = 0;
    
  for(int instIndex = 0; instIndex < instances.Size(); instIndex++){
    BLOCK_REGION(temp);

    FUInstance* inst = instances.Get(instIndex);

    FUDeclaration* decl = inst->declaration;
    if(decl == BasicDeclaration::input || decl == BasicDeclaration::output || decl->IsCombinatorialOperation()){
      continue;
    }

    m->StartInstance(decl->name,SF("%.*s_%d",UNPACK_SS(inst->name),instIndex));

    Hashmap<String,SymbolicExpression*>* params = GetParametersOfUnit(inst,temp);
    for(Pair<String,SymbolicExpression**> p : params){
      if(p.second){
        String repr = PushRepresentation(*p.second,temp);
        m->InstanceParam(CS(p.first),CS(repr));
      }
    }
    if(decl->memoryMapBits.has_value()){
      SymbolicExpression* expr = PushLiteral(temp,decl->memoryMapBits.value());
      params->Insert(S8("ADDR_W"),expr);
    }
    
    for(int i = 0; i < inst->outputs.size; i++){
      if(inst->outputs[i]){
        m->PortConnectIndexed("out%d",i,SF("output_%d_%d",instIndex,i));
      } else {
        m->PortConnectIndexed("out%d",i,"");
      }
    }

    for(int i = 0; i < inst->inputs.size; i++){
      if(inst->inputs[i].inst){
        PortInstance other = GetAssociatedOutputPortInstance(inst,i);
        m->PortConnectIndexed("in%d",i,OutputName(instances,other,temp));
      } else {
        m->PortConnectIndexed("in%d",i,"0");
      }
    }

    SymbolicExpression* configDataExpr = PushLiteral(temp,0);
    
    int configDataIndex = 0;
    for(Wire w : decl->configs){
      String repr = PushRepresentation(configDataExpr,temp);
      String size = PushRepresentation(w.sizeExpr,temp);
      
      m->PortConnect(w.name,PushString(temp,"configdata[(%.*s)+:%.*s]",UN(repr),UN(size)));
      
      configDataExpr = Normalize(SymbolicAdd(configDataExpr,w.sizeExpr,temp),temp);
      configDataIndex += w.bitSize;
    }

    for(Pair<StaticId,StaticData*> p : decl->staticUnits){
      StaticId id = p.first;
      for(Wire w : p.second->configs){
        String wireStaticName = PushString(temp,"%.*s_%.*s_%.*s",UNPACK_SS(id.parent->name),UNPACK_SS(id.name),UNPACK_SS(w.name));
          
        m->PortConnect(wireStaticName,wireStaticName);
      }
    }
    
    // Delays
    SymbolicExpression* delayExpr = val.delayStart;
    for(int i = 0; i < decl->numberDelays; i++){
      String repr = PushRepresentation(delayExpr,temp);

      m->PortConnectIndexed("delay%d",i,PushString(temp,"configdata[(%.*s)+:DELAY_W]",UN(repr)));
      delayExpr = Normalize(SymbolicAdd(delayExpr,PushVariable(temp,S8("DELAY_W")),temp),temp);
    }
    
    // State
    int stateIndex = 0;
    for(Wire w : decl->states){
      m->PortConnect(w.name,PushString(temp,"statedata[%d+:%d]",stateIndex,w.bitSize));
      stateIndex += w.bitSize;
    }

    // External memories
    for(int i = 0; i <  decl->externalMemory.size; i++){
      ExternalMemoryInterface ext = decl->externalMemory[i];
      if(ext.type == ExternalMemoryType::DP){
        m->PortConnectIndexed("ext_dp_addr_%d_port_0",i,"ext_dp_addr_%d_port_0_o",externalSeen);
        m->PortConnectIndexed("ext_dp_out_%d_port_0",i,"ext_dp_out_%d_port_0_o",externalSeen);
        m->PortConnectIndexed("ext_dp_in_%d_port_0",i,"ext_dp_in_%d_port_0_i",externalSeen);
        m->PortConnectIndexed("ext_dp_enable_%d_port_0",i,"ext_dp_enable_%d_port_0_o",externalSeen);
        m->PortConnectIndexed("ext_dp_write_%d_port_0",i,"ext_dp_write_%d_port_0_o",externalSeen);

        m->PortConnectIndexed("ext_dp_addr_%d_port_1",i,"ext_dp_addr_%d_port_1_o",externalSeen);
        m->PortConnectIndexed("ext_dp_out_%d_port_1",i,"ext_dp_out_%d_port_1_o",externalSeen);
        m->PortConnectIndexed("ext_dp_in_%d_port_1",i,"ext_dp_in_%d_port_1_i",externalSeen);
        m->PortConnectIndexed("ext_dp_enable_%d_port_1",i,"ext_dp_enable_%d_port_1_o",externalSeen);
        m->PortConnectIndexed("ext_dp_write_%d_port_1",i,"ext_dp_write_%d_port_1_o",externalSeen);
      } else {
        m->PortConnectIndexed("ext_2p_addr_out_%d",i,"ext_2p_addr_out_%d_o",externalSeen);
        m->PortConnectIndexed("ext_2p_addr_in_%d",i,"ext_2p_addr_in_%d_o",externalSeen);
        m->PortConnectIndexed("ext_2p_write_%d",i,"ext_2p_write_%d_o",externalSeen);
        m->PortConnectIndexed("ext_2p_read_%d",i,"ext_2p_read_%d_o",externalSeen);
        m->PortConnectIndexed("ext_2p_data_in_%d",i,"ext_2p_data_in_%d_i",externalSeen);
        m->PortConnectIndexed("ext_2p_data_out_%d",i,"ext_2p_data_out_%d_o",externalSeen);
      }
      externalSeen += 1;
    }

    // Memory mapping
    if(decl->memoryMapBits.has_value()){
      m->PortConnect("valid",SF("memoryMappedEnable[%d]",memoryMappedSeen));
      m->PortConnect("wstrb","data_wstrb");
      if(decl->memoryMapBits.value() > 0){
        m->PortConnect("addr",SF("csr_addr[%d-1:0]",decl->memoryMapBits.value()));
      }
      m->PortConnect("rdata",SF("unitRData[%d]",memoryMappedSeen));
      m->PortConnect("rvalid",SF("unitRValid[%d]",memoryMappedSeen));
      m->PortConnect("wdata","data_data");
        
      memoryMappedSeen += 1;
    }

    // Databus
    for(int i = 0; i < decl->nIOs; i++){
      m->PortConnectIndexed("databus_ready_%d",i,"databus_ready[%d]",ioSeen);
      m->PortConnectIndexed("databus_valid_%d",i,"databus_valid[%d]",ioSeen);
      m->PortConnectIndexed("databus_addr_%d",i,"databus_addr[%d]",ioSeen);
      m->PortConnectIndexed("databus_rdata_%d",i,"databus_rdata[%d]",ioSeen);
      m->PortConnectIndexed("databus_wdata_%d",i,"databus_wdata[%d]",ioSeen);
      m->PortConnectIndexed("databus_wstrb_%d",i,"databus_wstrb[%d]",ioSeen);
      m->PortConnectIndexed("databus_len_%d",i,"databus_len[%d]",ioSeen);
      m->PortConnectIndexed("databus_last_%d",i,"databus_last[%d]",ioSeen);

      ioSeen += 1;
    }

    if(decl->signalLoop){
      m->PortConnect("signal_loop","signal_loop");
    }

    m->PortConnect("running","running");
    m->PortConnect("run","run");

    if(decl->implementsDone){
      m->PortConnect("done",SF("unitDone[%d]",doneSeen++));
    }

    m->PortConnect("clk","clk");
    m->PortConnect("rst","rst");
      
    m->EndInstance();
  }
}

void EmitConnectOutputsToOut(VEmitter* v,Pool<FUInstance> instances){
  TEMP_REGION(temp,v->arena);

  FUInstance* outNode = nullptr;
  for(FUInstance* node : instances){
    if(node->declaration == BasicDeclaration::output){
      outNode = node;
    }
  }

  if(outNode){
    for(int i = 0; i < outNode->inputs.size; i++){
      if(!outNode->inputs[i].inst){
        continue;
      }
      
      PortInstance other = GetAssociatedOutputPortInstance(outNode,i);
      
      // TODO: Test this section very throughly
      v->Assign(PushString(temp,"out%d",i),OutputName(instances,other,temp));
    }
  }
}

void OutputCircuitSource(FUDeclaration* module,FILE* file){
  TEMP_REGION(temp,nullptr);
  
  Accelerator* accel = module->fixedDelayCircuit;
  AccelInfo info = module->info;

  Pool<FUInstance> instances = accel->allocated;
  
  Array<InstanceInfo*> allSameLevel = GetAllSameLevelUnits(&info,0,0,temp);
  auto builder = StartArray<Array<int>>(temp);
  for(InstanceInfo* p : allSameLevel){
    *builder.PushElem() = p->individualWiresGlobalConfigPos;
  }
  Array<Array<int>> wireIndexByInstanceGood = EndArray(builder);
  Array<Wire> configs = module->configs;
  
  VEmitter* m = StartVCode(temp);
  TEMP_REGION(temp2,m->arena);
  
  m->Timescale("1ns","1ps");

  m->Module(module->name);
  m->ModuleParam("AXI_ADDR_W",0);
  m->ModuleParam("AXI_DATA_W",0);
  m->ModuleParam("ADDR_W",0);
  m->ModuleParam("DATA_W",0);
  m->ModuleParam("DELAY_W",0);
  m->ModuleParam("LEN_W",0);

  m->Input("run");
  m->Input("running");
  m->Input("clk");
  m->Input("rst");

  if(info.nDones){
    m->Output("done");
  }
  if(module->signalLoop){
    m->Input("signal_loop");
  }
  
  for(int i = 0; i < module->NumberInputs(); i++){
    m->InputIndexed("in%d",i,"DATA_W");
  }

  for(int i = 0; i < module->NumberOutputs(); i++){
    m->OutputIndexed("out%d",i,"DATA_W");
  }

  if(module->configs.size) m->Blank();
  for(Wire w : module->configs){
    String repr = PushRepresentation(w.sizeExpr,temp);
    m->Input(CS(w.name),CS(repr));
  }

  for(Pair<StaticId,StaticData*> p : module->staticUnits){
    for(Wire w : p.second->configs){
      String repr = PushRepresentation(w.sizeExpr,temp);
      m->Input(CS(GlobalStaticWireName(p.first,w,temp2)),CS(repr));
    }
  }

  for(Wire w : module->states){
    m->Output(w.name,w.bitSize);
  }

  for(int i = 0; i < module->numberDelays; i++){
    m->InputIndexed("delay%d",i,"DELAY_W");
  }

  // Databus interface
  for(int i = 0; i < module->nIOs; i++){
    m->InputIndexed("databus_ready_%d",i);
    m->OutputIndexed("databus_valid_%d",i);
    m->OutputIndexed("databus_addr_%d",i,"AXI_ADDR_W");
    m->InputIndexed("databus_rdata_%d",i,"AXI_DATA_W");
    m->OutputIndexed("databus_wdata_%d",i,"AXI_DATA_W");
    m->OutputIndexed("databus_wstrb_%d",i,"(AXI_DATA_W/8)");
    m->OutputIndexed("databus_len_%d",i,"LEN_W");
    m->InputIndexed("databus_last_%d",i);
  }
  
  // External Memory interface
  for(int i = 0; i <  module->externalMemory.size; i++){
    ExternalMemoryInterface ext  =  module->externalMemory[i];
    if(ext.type == ExternalMemoryType::DP){
      m->OutputIndexed("ext_dp_addr_%d_port_0",i,ext.dp[0].bitSize);
      m->OutputIndexed("ext_dp_out_%d_port_0",i,ext.dp[0].dataSizeOut);
      m->InputIndexed("ext_dp_in_%d_port_0",i,ext.dp[0].dataSizeIn);
      m->OutputIndexed("ext_dp_enable_%d_port_0",i);
      m->OutputIndexed("ext_dp_write_%d_port_0",i);

      m->OutputIndexed("ext_dp_addr_%d_port_1",i,ext.dp[1].bitSize);
      m->OutputIndexed("ext_dp_out_%d_port_1",i,ext.dp[1].dataSizeOut);
      m->InputIndexed("ext_dp_in_%d_port_1",i,ext.dp[1].dataSizeIn);
      m->OutputIndexed("ext_dp_enable_%d_port_1",i);
      m->OutputIndexed("ext_dp_write_%d_port_1",i);
    } else {
      m->OutputIndexed("ext_2p_addr_out_%d",i,ext.tp.bitSizeOut);
      m->OutputIndexed("ext_2p_addr_in_%d",i,ext.tp.bitSizeIn);
      m->OutputIndexed("ext_2p_write_%d",i);
      m->OutputIndexed("ext_2p_read_%d",i);
      m->InputIndexed("ext_2p_data_in_%d",i,ext.tp.dataSizeIn);
      m->OutputIndexed("ext_2p_data_out_%d",i,ext.tp.dataSizeOut);
    }
  }

  if(module->memoryMapBits){
    m->Input("valid");
    if(module->memoryMapBits.value() > 0){
      m->Input("addr",module->memoryMapBits.value());
    }
    m->Input("wstrb","(DATA_W/8)");
    m->Input("wdata","DATA_W");
    m->Output("rvalid");
    m->Output("rdata","DATA_W");
  }

  // Memory mapped units
  if(info.unitsMapped >= 1){
    Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

    m->Wire("unitRValid",info.unitsMapped);
    m->Assign("rvalid","(|unitRValid)");

    m->Reg("memoryMappedEnable",memoryMasks.size);

    m->WireArray("unitRData",info.unitsMapped,"DATA_W");
    m->WireAndAssignJoinBlock("unitRDataFinal","|","DATA_W");
    for(int i = 0; i < info.unitsMapped; i++){
      m->Expression(SF("unitRData[%d]",i));
    }
    m->EndBlock();
    m->Assign("rdata","unitRDataFinal");

    m->CombBlock();
    {
      m->Set("memoryMappedEnable",SF("%d'b0",info.unitsMapped));
      m->If("valid");
      for(int i = 0; i <  memoryMasks.size; i++){
        String mask  =  memoryMasks[i];
        if(!Empty(mask)){
          m->If(SF("addr[(%d-1) -: %d] == %d'b%.*s",module->memoryMapBits.value(),mask.size,mask.size,UNPACK_SS(mask)));
          m->Set(SF("memoryMappedEnable[%d]",i),"1'b1");
          m->EndIf();
        } else {
          m->Set(SF("memoryMappedEnable[%d]",i),"1'b1");
        }
      }
      m->EndIf();
    }
    m->EndBlock();
  }

  if(info.nDones){
    m->Wire("unitDone",info.nDones);
    m->Assign("done","&unitDone");
  }

  if(info.numberConnections){
    for(int i = 0; i < instances.Size(); i++){
      FUInstance* node = instances.Get(i);
      for(int k = 0; k < node->outputs.size; k++){
        bool out = node->outputs[k];
        if(out){
          m->Wire(SF("output_%d_%d",i,k),"DATA_W");
        }
      }
    }
  }
  
  EmitCombOperations(m,instances);

  m->Blank();
  EmitInstanciateUnits(m,instances,module,wireIndexByInstanceGood,configs);

  EmitConnectOutputsToOut(m,instances);

  m->EndModule();
  
  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(temp,b);

  fprintf(file,"%.*s",UNPACK_SS(content));
}

String EmitConfiguration(VersatComputedValues val,Array<WireInformation> wireInfo,Arena* out){
  TEMP_REGION(temp,out);
    
  VEmitter* m = StartVCode(temp);

  // VARS
  int configurationsBits = val.configurationBits;
  int configurationAddressBits = val.configurationAddressBits;
  
  m->Timescale("1ns","1ps");

  m->Include("versat_defs.vh");

  m->Module(STRING("versat_configurations"));
  {
    // TODO: We probably want to generate this from the parameters of the declaration, instead of hardcoded like this.
    m->ModuleParam("ADDR_W",32);
    m->ModuleParam("DATA_W",32);
    m->ModuleParam("AXI_ADDR_W",32);
    m->ModuleParam("AXI_DATA_W",globalOptions.databusDataSize);
    m->ModuleParam("LEN_W",20);
    m->ModuleParam("DELAY_W",7);

    if(configurationsBits){
      String repr = PushRepresentation(val.configurationBitsExpr,temp);

      m->Output("config_data_o",CS(repr));
      
      m->Reg("configdata",repr);

      if(globalOptions.shadowRegister){
        m->Reg("shadow_configdata",repr);
      }
    }

    m->Input("memoryMappedAddr");
    m->Input("data_write");

    m->Input("address","ADDR_W");
    m->Input("data_wstrb","(DATA_W/8)");
    m->Input("data_data","DATA_W");

    m->Input("change_config_pulse");

    for(WireInformation info : wireInfo){
      if(info.isStatic){
        String repr = PushRepresentation(info.wire.sizeExpr,temp);
        m->Output(CS(info.wire.name),CS(repr));
      }
    }

    m->Input("clk_i");
    m->Input("rst_i");

    m->Assign("config_data_o","configdata");

    for(WireInformation info : wireInfo){
      String repr = PushRepresentation(info.wire.sizeExpr,temp);
      Wire w = info.wire;
      if(w.stage == VersatStage_COMPUTE || w.stage == VersatStage_WRITE){
        m->Reg(SF("Compute_%.*s",UN(w.name)),repr);
      }
      if(w.stage == VersatStage_WRITE){
        m->Reg(SF("Write_%.*s",UN(w.name)),repr);
      }
    }

    // TODO: Need to handle endianess
    auto EmitStrobe = [](VEmitter* m,const char* strobeWire,const char* leftReg,String leftIndexStart,const char* rightReg,int regSize){
      for(int i = 0; i < regSize; i += 8){
        int left = 8;
        if(i + left > regSize){
          left = regSize % 8;
        }
        
        m->If(SF("%s[%d]",strobeWire,i/8));
        m->Set(SF("%s[(%.*s)+%d+:%d]",leftReg,UN(leftIndexStart),i,left),SF("%s[%d+:%d]",rightReg,i,left));
        m->EndIf();
      }
    };
    
    if(configurationsBits){
      m->Comment("Data to store to shadow config");
      m->Integer("i");
      m->AlwaysBlock("clk_i","rst_i");
      {
        m->If("rst_i");
        m->Set("shadow_configdata",0);
        m->ElseIf("data_write & !memoryMappedAddr");
        
        for(WireInformation info : wireInfo){
          m->If(SF("address[%d:0] == %d",configurationAddressBits + 1,info.addr));

          String configStartExpr = PushRepresentation(info.bitExpr,temp);
          
          if(info.wire.sizeExpr && info.wire.sizeExpr->type != SymbolicExpressionType_LITERAL){
            String repr = PushRepresentation(info.wire.sizeExpr,temp);
            
            // TODO: Need to handle endianess
            m->Loop("i = 0",SF("i < %.*s",UN(repr)),"i++");
            m->If(SF("data_wstrb[i/8]"));
            m->Set(SF("shadow_configdata[(%.*s)+i]",UN(configStartExpr)),SF("data_data[i]"));
            m->EndIf();
            m->EndLoop();
          } else {
            EmitStrobe(m,"data_wstrb","shadow_configdata",configStartExpr,"data_data",info.wire.bitSize);
          }
          
          m->EndIf();
        }
        m->EndIf();
      }
      m->EndBlock();
      
      m->Comment("Shadow config to config");
      m->AlwaysBlock("clk_i","rst_i");
      {
        m->If("rst_i");
        m->Set("configdata",0);
        m->ElseIf("change_config_pulse");
        for(WireInformation info : wireInfo){
          String start = PushRepresentation(info.bitExpr,temp);
          String size = PushRepresentation(info.wire.sizeExpr,temp);
          
          if(info.wire.stage == VersatStage_READ){
            m->Set(SF("configdata[(%.*s)+:%.*s]",UN(start),UN(size)),SF("shadow_configdata[(%.*s)+:%.*s]",UN(start),UN(size)));
          }
        }
        for(WireInformation info : wireInfo){
          String start = PushRepresentation(info.bitExpr,temp);
          String size = PushRepresentation(info.wire.sizeExpr,temp);

          if(info.wire.stage == VersatStage_COMPUTE || info.wire.stage == VersatStage_WRITE){
            m->Set(SF("configdata[%.*s+:%.*s]",UN(start),UN(size)),SF("Compute_%.*s",UN(info.wire.name)));
          }
        }
        m->EndIf();
      }
      m->EndBlock();
    }

    m->Comment("Compute");
    m->AlwaysBlock("clk_i","rst_i");
    {
      m->If("rst_i");
      for(WireInformation info : wireInfo){
        if(info.wire.stage == VersatStage_COMPUTE || info.wire.stage == VersatStage_WRITE){
          m->Set(SF("Compute_%.*s",UN(info.wire.name)),"0");
        }
      }
      m->ElseIf("change_config_pulse");
      for(WireInformation info : wireInfo){
        String start = PushRepresentation(info.bitExpr,temp);
        String size = PushRepresentation(info.wire.sizeExpr,temp);
        if(info.wire.stage == VersatStage_COMPUTE){
          m->Set(SF("Compute_%.*s",UN(info.wire.name)),SF("shadow_configdata[(%.*s)+:%.*s]",UN(start),UN(size)));
        }
      }
      for(WireInformation info : wireInfo){
        if(info.wire.stage == VersatStage_WRITE){
          m->Set(SF("Compute_%.*s",UN(info.wire.name)),SF("Write_%.*s",UN(info.wire.name)));
        }
      }
      m->EndIf();
    }
    m->EndBlock();

    m->Comment("Write");
    m->AlwaysBlock("clk_i","rst_i");
    {
      m->If("rst_i");
      for(WireInformation info : wireInfo){
        if(info.wire.stage == VersatStage_WRITE){
          m->Set(SF("Write_%.*s",UN(info.wire.name)),"0");
        }
      }
      m->ElseIf("change_config_pulse");
      for(WireInformation info : wireInfo){
        String start = PushRepresentation(info.bitExpr,temp);
        String size = PushRepresentation(info.wire.sizeExpr,temp);
        if(info.wire.stage == VersatStage_WRITE){
          m->Set(SF("Write_%.*s",UN(info.wire.name)),SF("shadow_configdata[(%.*s)+:%.*s]",UN(start),UN(size)));
        }
      }
      m->EndIf();
    }
    m->EndBlock();

    for(WireInformation info : wireInfo){
      if(info.isStatic){
        String start = PushRepresentation(info.bitExpr,temp);
        m->Assign(info.wire.name,PushString(temp,"configdata[(%.*s)+:%d]",UN(start),info.wire.bitSize));
      }
    }
  }
  m->EndModule();
  
  VAST* ast = EndVCode(m);
  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String content = EndString(out,b);
  return content;
}

Array<Difference> CalculateSmallestDifference(Array<int> oldValues,Array<int> newValues,Arena* out){
  Assert(oldValues.size == newValues.size); // For now

  int size = oldValues.size;

  auto arr = StartArray<Difference>(out);
  for(int i = 0; i < size; i++){
    int oldVal = oldValues[i];
    int newVal = newValues[i];

    if(oldVal != newVal){
      Difference* dif = arr.PushElem();

      dif->index = i;
      dif->newValue = newVal;
    }
  }

  Array<Difference> result = EndArray(arr);
  return result;
}

Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out){
  TEMP_REGION(temp,out);

  int size = types.size;

  int stored = 0;
  Array<FUDeclaration*> result = PushArray<FUDeclaration*>(out,size);

  Hashmap<FUDeclaration*,bool>* seen = PushHashmap<FUDeclaration*,bool>(temp,size);
  Array<Array<FUDeclaration*>> subTypes = PushArray<Array<FUDeclaration*>>(temp,size);
  Memset(subTypes,{});

  for(int i = 0; i < size; i++){
    subTypes[i] = MemSubTypes(&types[i]->info,temp);
    //subTypes[i] = MemSubTypes(types[i]->fixedDelayCircuit,temp); // TODO: We can reuse the SortTypesByConfigDependency function if we change it to receive the subTypes array from outside, since the rest of the code is equal.
    seen->Insert(types[i],false);
  }

  for(int iter = 0; iter < size; iter++){
    bool breakEarly = true;
    for(int i = 0; i < size; i++){
      FUDeclaration* type = types[i];
      Array<FUDeclaration*> sub = subTypes[i];

      bool* seenType = seen->Get(type);

      if(seenType && *seenType){
        continue;
      }

      bool allSeen = true;
      for(FUDeclaration* subIter : sub){
        bool* res = seen->Get(subIter);

        if(res && !*res){
          allSeen = false;
          break;
        }
      }

      if(allSeen){
        *seenType = true;
        result[stored++] = types[i];
        breakEarly = false;
      }
    }

    if(breakEarly){
      break;
    }
  }

  for(Pair<FUDeclaration*,bool*> p : seen){
    Assert(*p.second);
  }

  return result;
}

static Array<TypeStructInfoElement> GenerateAddressStructFromType(FUDeclaration* decl,Arena* out){
  if(decl->type == FUDeclarationType_SINGLE){
    int size = 1;
    Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,size);

    for(int i = 0; i < size; i++){
      entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
      entries[i].typeAndNames[0].type = STRING("void*");
      entries[i].typeAndNames[0].name = STRING("addr");
    }
    return entries;
  }

  int memoryMapped = 0;
  for(FUInstance* node : decl->fixedDelayCircuit->allocated){
    FUDeclaration* decl = node->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    memoryMapped += 1;
  }

  Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,memoryMapped);

  int index = 0;
  for(FUInstance* node : decl->fixedDelayCircuit->allocated){
    FUDeclaration* decl = node->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    entries[index].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
    entries[index].typeAndNames[0].type = PushString(out,"%.*sAddr",UNPACK_SS(decl->name));
    entries[index].typeAndNames[0].name = node->name;
    index += 1;
  }
  return entries;
}

static Array<TypeStructInfo> GetMemMappedStructInfo(AccelInfo* info,Arena* out){
  TEMP_REGION(temp,out);

  Array<FUDeclaration*> typesUsed = MemSubTypes(info,out);
  typesUsed = SortTypesByMemDependency(typesUsed,temp);

  Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(out,typesUsed.size);
  int index = 0;
  for(auto& decl : typesUsed){
    Array<TypeStructInfoElement> val = GenerateAddressStructFromType(decl,out);
    structures[index].name = decl->name;
    structures[index].entries = val;
    index += 1;
  }

  return structures;
}

Array<TypeStructInfoElement> ExtractStructuredConfigs(Array<InstanceInfo> info,Arena* out){
  TEMP_REGION(temp,out);

  TrieMap<int,ArenaList<String>*>* map = PushTrieMap<int,ArenaList<String>*>(temp);
  
  int maxConfig = 0;
  for(InstanceInfo& in : info){
    if(in.isComposite || !in.globalConfigPos.has_value() || in.isConfigStatic){
      continue;
    }
    
    for(int i = 0; i < in.configSize; i++){
      int config = in.individualWiresGlobalConfigPos[i];

      maxConfig = std::max(maxConfig,config);
      GetOrAllocateResult<ArenaList<String>*> res = map->GetOrAllocate(config);

      if(!res.alreadyExisted){
        *res.data = PushArenaList<String>(temp);
      }

      ArenaList<String>* list = *res.data;
      String name = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->configs[i].name));

      // Quick and dirty way of skipping same name
      bool skip = false;
      for(SingleLink<String>* ptr = list->head; ptr; ptr = ptr->next){
        if(CompareString(ptr->elem,name)){
          skip = true;
        }
      }
      // Cannot put same name since header file gives collision later on.
      if(skip){
        continue;
      }
      
      *list->PushElem() = name;
    }
  }
  
  int configSize = maxConfig + 1;
  ArenaList<TypeStructInfoElement>* elems = PushArenaList<TypeStructInfoElement>(temp);
  for(int i = 0; i < configSize; i++){
    ArenaList<String>** optList = map->Get(i);
    if(!optList){
      continue;
    }

    ArenaList<String>* list = *optList;
    int size = Size(list);

    Array<SingleTypeStructElement> arr = PushArray<SingleTypeStructElement>(out,size);
    int index = 0;
    for(SingleLink<String>* ptr = list->head; ptr; ptr = ptr->next){
      String elem = ptr->elem;
      arr[index].type = STRING("iptr");
      arr[index].name = elem;
      
      index += 1;
    }

    elems->PushElem()->typeAndNames = arr;
  }
  
  return PushArrayFromList(out,elems);
}

void OutputIterativeSource(FUDeclaration* decl,FILE* file){
  printf("\n\nError, output of iterative units is currently disabled\n\n");
  NOT_IMPLEMENTED("");
  return;
  
#if 0
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  // NOTE: Iteratives have not been properly maintained.
  //       Most of this code probably will not work

  Accelerator* accel = decl->fixedDelayCircuit;

  ClearTemplateEngine(); // Make sure that we do not reuse data

  Assert(globalDebug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set
  
  // TODO: If we get iterative working again, this should just get the AccelInfo from the decl.
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp);

  // TODO: If going back to iteratives, remove this. Only Top level should care about Versat Values.
  VersatComputedValues val = ComputeVersatValues(&info,false);

  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(&info,temp);
  
  Pool<FUInstance> nodes = accel->allocated;
  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }
  
  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);

  TemplateSetCustom("topLevel",MakeValue(&topLevelUnits));
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetCustom("instances",MakeValue(&nodes));
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
    
  int lat = decl->lat;
  TemplateSetNumber("special",lat);

  ProcessTemplate(file,BasicTemplates::iterativeTemplate);
#endif
}

// TODO: Move to a better place
bool ContainsPartialShare(InstanceInfo* info){
  bool seenFalse = false;
  bool seenTrue = false;
  for(bool b : info->individualWiresShared){
    if(b){
      seenTrue = true;
    }
    if(!b){
      seenFalse = true;
    }
  }

  return (seenTrue && seenFalse);
}

// TODO: Do not know if we need to merge the config and state flows.
//       But something should be done about the integer hack below.
//       Feels kinda weird. Some changes in the data structures might simplify the code somewhat.
StructInfo* GenerateStateStruct(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);

  StructInfo* res = PushStruct<StructInfo>(out);

  InstanceInfo* parent = iter.GetParentUnit();
  if(parent){
    res->name = parent->decl->name;
  }

  // TODO: HACK
  static StructInfo integer = {};
  integer.name = STRING("iptr");
  
  auto list = PushArenaDoubleList<StructElement>(out);
  for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
    InstanceInfo* unit = it.CurrentUnit();
    StructElement elem = {};

    if(!unit->statePos.has_value()){
      continue;
    }

    int parentGlobalStatePos = 0;
    if(parent){
      parentGlobalStatePos = parent->statePos.value();
    }
    
    elem.name = unit->baseName;

    if(unit->isMerge){
      auto GenerateMergeStruct = [](InstanceInfo* topUnit,AccelInfoIterator iter,Arena* out)->StructInfo*{
        auto list = PushArenaDoubleList<StructElement>(out);
        StructInfo* res = PushStruct<StructInfo>(out);
        
        StructElement elem = {};
        for(int i = 0; i < iter.MergeSize(); i++){
          iter.SetMergeIndex(i);
        
          StructInfo* subInfo = GenerateStateStruct(iter,out);

          // NOTE: Why config version does not do this?
          subInfo->name = iter.GetMergeName();
          
          elem.name = iter.GetMergeName();
          elem.childStruct = subInfo;
          elem.localPos = 0;
          elem.size = topUnit->stateSize;

          *list->PushElem() = elem;
        }
        
        res->name = topUnit->decl->name;
        res->memberList = list;
        
        return res;
      };

      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateMergeStruct(unit,inside,out);
      elem.childStruct = subInfo;
    } else if(unit->isComposite){
      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateStateStruct(inside,out);
      elem.childStruct = subInfo;
    } else {
      StructInfo* simpleSubInfo = PushStruct<StructInfo>(out);

      simpleSubInfo->name = unit->decl->name;
      
      ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
      int index = 0;
      Array<Wire> states = unit->decl->states;
      for(Wire w : states){
        StructElement* elem = elements->PushElem();
        elem->name = w.name;
        elem->localPos = index++;
        elem->isMergeMultiplexer = false;
        elem->size = 1;
        elem->childStruct = &integer;
      }
      simpleSubInfo->memberList = elements;
      
      elem.childStruct = simpleSubInfo;
    }

    // TODO: Kinda of an hack because we do not have local state pos the same way we have local config pos.
    elem.localPos = unit->statePos.value() - parentGlobalStatePos;
    elem.size = unit->stateSize;
    elem.doesNotBelong = unit->doesNotBelong;
    
    *list->PushElem() = elem;
  }
  
  res->memberList = list;
  
  return res;
}

StructInfo* GenerateConfigStructRecurse(AccelInfoIterator iter,TrieMap<StructInfo,StructInfo*>* generatedStructs,Arena* out){
  StructInfo res = {};//PushStruct<StructInfo>(out);

  InstanceInfo* parent = iter.GetParentUnit();
 
  if(parent && parent->isMerge){
    res.name = iter.GetMergeName();
  } else if(parent){
    res.name = parent->decl->name;
  }

  // TODO: HACK
  static StructInfo integer = {};
  integer.name = STRING("iptr");
  
  // This function returns one struct info.
  auto list = PushArenaDoubleList<StructElement>(out);
  for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
    InstanceInfo* unit = it.CurrentUnit();
    StructElement elem = {};
      
    if(!unit->globalConfigPos.has_value()){
      continue;
    }
    
    int unitLocalPos = unit->localConfigPos.value();
    
    elem.name = unit->baseName;

    if(unit->isMerge){
      // Merge struct is different, we do not iterate members but instead iterate the base types.
      auto GenerateMergeStruct =[](InstanceInfo* topUnit,AccelInfoIterator iter,TrieMap<StructInfo,StructInfo*>* generatedStructs,Arena* out)->StructInfo*{
        auto list = PushArenaDoubleList<StructElement>(out);
        StructInfo res = {}; //PushStruct<StructInfo>(out);
        
        StructElement elem = {};
        for(int i = 0; i < iter.MergeSize(); i++){
          iter.SetMergeIndex(i);
        
          StructInfo* subInfo = GenerateConfigStructRecurse(iter,generatedStructs,out);

          elem.name = iter.GetMergeName();
          elem.childStruct = subInfo;
          elem.localPos = 0; // Zero because the merge struct is basically a wrapper. 
          elem.size = topUnit->configSize;
          elem.isMergeMultiplexer = topUnit->isMergeMultiplexer;

          *list->PushElem() = elem;
        }
        
        res.name = topUnit->decl->name;
        res.originalName = res.name;
        res.memberList = list;

        auto result = generatedStructs->GetOrAllocate(res);
        if(!result.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = res;
          
          *result.data = allocated;
        }
        
        return *result.data;
      };

      AccelInfoIterator inside = it.StepInsideOnly();

      StructInfo* subInfo = GenerateMergeStruct(unit,inside,generatedStructs,out);
      elem.childStruct = subInfo;
    } else if(unit->isComposite){
      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateConfigStructRecurse(inside,generatedStructs,out);
      elem.childStruct = subInfo;
    } else {
      // NOTE: We are generating the sub struct info directly here instead of recursing.
      Array<int> localPos = unit->individualWiresLocalConfigPos;
      
      if(ContainsPartialShare(unit)){
        StructInfo simpleSubInfo = {};
        
        Array<Wire> configs = unit->decl->configs;
        
        ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
        int index = 0;

        Array<int> localPos = unit->individualWiresLocalConfigPos;
        for(Wire w : configs){
          StructElement* elem = elements->PushElem();
          elem->name = w.name;
          elem->localPos = localPos[index++] - unitLocalPos;

          Assert(elem->localPos >= 0);
          
          elem->isMergeMultiplexer = false;
          elem->size = 1;
          elem->childStruct = &integer;
        }
        
        simpleSubInfo.name = unit->decl->name;
        simpleSubInfo.originalName = unit->decl->name;
        simpleSubInfo.memberList = elements;

        auto res = generatedStructs->GetOrAllocate(simpleSubInfo);
        if(!res.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = simpleSubInfo;
          
          *res.data = allocated;
        }

        elem.childStruct = *res.data;
      } else {
        // NOTE: This structure should be the same accross all the invocations of this function, since we are just generating the struct of a simple type without any struct wide modifications (like partial share or merge).

        // NOTE: As such, we can detect in here wether we are generating something that already exists or not. Instead of using the TrieMap approach later in the ExtractStructs.
        StructInfo simpleSubInfo = {};
        
        Array<Wire> configs = unit->decl->configs;
        
        ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
        int index = 0;
        for(Wire w : configs){
          StructElement* elem = elements->PushElem();
          elem->name = w.name;
          elem->localPos = localPos[index++] - unitLocalPos;

          Assert(elem->localPos >= 0);

          elem->isMergeMultiplexer = false;
          elem->size = 1;
          elem->childStruct = &integer;
        }
        
        simpleSubInfo.name = unit->decl->name;
        simpleSubInfo.originalName = unit->decl->name;
        simpleSubInfo.memberList = elements;

        auto res = generatedStructs->GetOrAllocate(simpleSubInfo);
        if(!res.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = simpleSubInfo;
          
          *res.data = allocated;
        }

        elem.childStruct = *res.data;
      }
    }

    int maxPos = 0;
    for(int i : unit->individualWiresLocalConfigPos){
      maxPos = MAX(maxPos,i);
    }

    elem.localPos = unitLocalPos;
    elem.size = maxPos - unitLocalPos + 1;

    Assert(elem.size > 0);

    elem.isMergeMultiplexer = unit->isMergeMultiplexer;
    elem.doesNotBelong = unit->doesNotBelong;

    unit->structInfo = elem.childStruct;
    
    *list->PushElem() = elem;
  }
      
  res.originalName = res.name;
  res.memberList = list;

  auto result = generatedStructs->GetOrAllocate(res);
  if(!result.alreadyExisted){
    StructInfo* allocated = PushStruct<StructInfo>(out);
    *allocated = res;
          
    *result.data = allocated;
  }
  
  return *result.data;
}

StructInfo* GenerateConfigStruct(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);

  // NOTE: It is important that same structs are "shared" because later we might change the name of the struct in order to avoid naming conflicts and by sharing the same struct every "user" of that struct gets updated at the same time.
  TrieMap<StructInfo,StructInfo*>* generatedStructs = PushTrieMap<StructInfo,StructInfo*>(temp);

  StructInfo* result = GenerateConfigStructRecurse(iter,generatedStructs,out);

  for(; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->structInfo){
      unit->structInfo = generatedStructs->GetOrFail(*unit->structInfo);
    }
  }
  
  return result;
}
  
size_t HashStructInfo(StructInfo* info){
  if(!info) { return 0;};
  return std::hash<StructInfo>()(*info);
}

Array<StructInfo*> ExtractStructs(StructInfo* structInfo,Arena* out){
  TEMP_REGION(temp,out);

  // NOTE: Because we share the structs that are equal, we could technically implement this as just a pointer triemap, insteado of hashing the entire struct. That is because we are sure that the same pointer is equivalent to same struct, but for now leave this as it is.
  TrieMap<StructInfo,StructInfo*>* info = PushTrieMap<StructInfo,StructInfo*>(temp);

  auto Recurse = [](auto Recurse,StructInfo* top,TrieMap<StructInfo,StructInfo*>* map) -> void {
    if(!top){
      return;
    }

    for(DoubleLink<StructElement>* ptr = top->memberList ? top->memberList->head : nullptr; ptr; ptr = ptr->next){
      Recurse(Recurse,ptr->elem.childStruct,map);
    }
    // TODO: HACK
    if(top->name == STRING("iptr")){
      return;
    }
    
    map->InsertIfNotExist(*top,top);
  };

  Recurse(Recurse,structInfo,info);
  
  Array<StructInfo*> res = PushArrayFromTrieMapData(out,info);

  return res;
}

// typeString - Config or State
Array<TypeStructInfo> GenerateStructs(Array<StructInfo*> info,String typeString,bool useIptr /* TODO: HACK */,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<TypeStructInfo>* list = PushArenaList<TypeStructInfo>(temp);

  for(StructInfo* structInfo : info){
    TypeStructInfo* type = list->PushElem();
    type->name = structInfo->name;

    int maxPos = 0;
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;
      maxPos = MAX(maxPos,elem.localPos + elem.size);
    }
    
    Array<int> amountOfEntriesAtPos = PushArray<int>(temp,maxPos);
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;
      amountOfEntriesAtPos[elem.localPos] += 1;
    }

    Array<bool> validPositions = PushArray<bool>(temp,maxPos);
    Array<bool> startPositionsOnly = PushArray<bool>(temp,maxPos);
    
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;

      startPositionsOnly[elem.localPos] = true;
      for(int i = elem.localPos; i < elem.localPos + elem.size; i++){
        validPositions[i] = true;
      }
    }

    auto builder = StartArray<IndexInfo>(temp);
    for(int i = 0; i < maxPos; i++){
      if(!validPositions[i]){
        *builder.PushElem() = {true,i,0};
      } else if(startPositionsOnly[i]){
        *builder.PushElem() = {false,i,amountOfEntriesAtPos[i]};
      }
    }
    Array<IndexInfo> indexInfo = EndArray(builder);

    // TODO: This could just be an array.
    TrieMap<int,int>* positionToIndex = PushTrieMap<int,int>(temp);
    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      if(!info.isPadding){
        positionToIndex->Insert(info.pos,i);
      }
    }
    
    type->entries = PushArray<TypeStructInfoElement>(out,indexInfo.size);

    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      type->entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,info.amountOfEntries);
    }

    Array<int> indexes = PushArray<int>(temp,maxPos);
    
    int paddingAdded = 0;
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr;){
      StructElement elem = ptr->elem;
      int pos = elem.localPos;

      int index = positionToIndex->GetOrFail(pos);
      
      int subIndex = indexes[pos]++;
      type->entries[index].typeAndNames[subIndex].name = elem.name;

      if(elem.doesNotBelong){
        type->entries[index].typeAndNames[subIndex].name = PushString(out,"padding_%d",paddingAdded++);
      }
      
      // TODO: HACK
      if(elem.childStruct->name == STRING("iptr")){
        if(useIptr) {
          type->entries[index].typeAndNames[subIndex].type = STRING("iptr");
        } else {
          type->entries[index].typeAndNames[subIndex].type = STRING("int");
        }
      } else {
        type->entries[index].typeAndNames[subIndex].type = PushString(out,"%.*s%.*s",UNPACK_SS(elem.childStruct->name),UNPACK_SS(typeString));
      }
      type->entries[index].typeAndNames[subIndex].arraySize = 0;

      ptr = ptr->next;
    }

    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      if(info.isPadding){
        int pos = info.pos;
        type->entries[pos].typeAndNames = PushArray<SingleTypeStructElement>(out,1);

        // TODO: HACK
        if(useIptr) {
          type->entries[pos].typeAndNames[0].type = STRING("iptr");
        } else {
          type->entries[pos].typeAndNames[0].type = STRING("int");
        }
        type->entries[pos].typeAndNames[0].name = PushString(out,"padding_%d",paddingAdded++);
      }
    }

    for(int i = 0; i < type->entries.size; i++){
      Assert(type->entries[i].typeAndNames.size > 0);
    }
  }
  
  Array<TypeStructInfo> res = PushArrayFromList(out,list);
  return res;
}

// NOTE: Does sharing the struct info cause problems here?
void PushMergeMultiplexersUpTheHierarchy(StructInfo* top){
  auto PushMergesUp = [](StructInfo* parent,StructElement* parentElem,StructInfo* child) -> void{
    if(child == nullptr || child->memberList == nullptr || child->memberList->head == nullptr){
      return;
    }

    PushMergeMultiplexersUpTheHierarchy(child);
    
    for(DoubleLink<StructElement>* childPtr = child->memberList->head; childPtr;){
      if(childPtr->elem.isMergeMultiplexer){
        DoubleLink<StructElement>* node = childPtr;

        int nodeActualPos = parentElem->localPos + childPtr->elem.localPos;
        
        childPtr = RemoveNodeFromList(child->memberList,node);

        // TODO: We should do a full comparison.
        //       For now only name because I already know muxes will have different names if different.
        bool sameName = false;
        for(DoubleLink<StructElement>* parentPtr = parent->memberList->head; parentPtr; parentPtr = parentPtr->next){
          sameName |= CompareString(parentPtr->elem.name,node->elem.name);
        }

        if(!sameName){
          node->elem.localPos = nodeActualPos;
          parent->memberList->PushNode(node);
        }
      } else {
        childPtr = childPtr->next;
      }
    }

  };
  
  for(DoubleLink<StructElement>* ptr = top->memberList->head; ptr;){
    PushMergesUp(top,&ptr->elem,ptr->elem.childStruct);

    if(ptr->elem.childStruct->memberList && Empty(ptr->elem.childStruct->memberList)){
      ptr = RemoveNodeFromList(top->memberList,ptr);
    } else {
      ptr = ptr->next;
    }
  }
}

void EmitIOUnpacking(VEmitter* m,int arraySize,Array<VerilogInterfaceSpec> spec,String unpackBase,String packedBase){
  if(arraySize == 0){
    return;
  }

  TEMP_REGION(temp,m->arena);
      
  for(VerilogInterfaceSpec info : spec){
    String unpackedName = PushString(temp,"%.*s_%.*s",UN(unpackBase),UN(info.name));

    if(Empty(info.sizeExpr)){
      m->WireArray(CS(unpackedName),arraySize,1);
    } else {
      m->WireArray(CS(unpackedName),arraySize,CS(info.sizeExpr));
    }
  }
      
  for(int i = 0; i < arraySize; i++){
    for(VerilogInterfaceSpec info : spec){

      String unpackedName = PushString(temp,"%.*s_%.*s",UN(unpackBase),UN(info.name));
      String packedName = PushString(temp,"%.*s_%.*s",UN(packedBase),UN(info.name));
          
      if(Empty(info.sizeExpr)){
        if(info.isShared){
          String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
          if(info.isInput){
            m->Assign(fullUnpacked,packedName);
          } else {
            m->Assign(packedName,fullUnpacked);
          }
        } else {
          if(info.isInput){
            String fullPacked = PushString(temp,"%.*s[%d]",UN(packedName),i);
            String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
            m->Assign(fullUnpacked,fullPacked);
          } else {
            String fullPacked = PushString(temp,"%.*s[%d]",UN(packedName),i);
            String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
            m->Assign(fullPacked,fullUnpacked);
          }
        }
      } else {
        if(info.isShared){
          String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
          if(info.isInput){
            m->Assign(fullUnpacked,packedName);
          } else {
            m->Assign(packedName,fullUnpacked);
          }
        } else {
          if(info.isInput){
            String fullPacked = PushString(temp,"%.*s[(%d * %.*s) +: %.*s]",UN(packedName),i,UN(info.sizeExpr),UN(info.sizeExpr));
            String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
            m->Assign(fullUnpacked,fullPacked);
          } else {
            String fullPacked = PushString(temp,"%.*s[(%d * %.*s) +: %.*s]",UN(packedName),i,UN(info.sizeExpr),UN(info.sizeExpr));
            String fullUnpacked = PushString(temp,"%.*s[%d]",UN(unpackedName),i);
            m->Assign(fullPacked,fullUnpacked);
          }
        }
      }
    }
  }
}

static void OutputMakefile(String typeName,FUDeclaration* topLevelDecl,String softwarePath){
  TEMP_REGION(temp,nullptr);
  
  String verilatorMakePath = PushString(temp,"%.*s/VerilatorMake.mk",UN(softwarePath));
  FILE* output = OpenFileAndCreateDirectories(verilatorMakePath,"w",FilePurpose_MAKEFILE);
  DEFER_CLOSE_FILE(output);

  region(temp){    
    bool simulateLoops = true;
  
    Array<String> allFilenames = PushArray<String>(temp,globalOptions.verilogFiles.size);
    for(int i = 0; i <  globalOptions.verilogFiles.size; i++){
      String filepath  =  globalOptions.verilogFiles[i];
      fs::path path(SF("%.*s",UNPACK_SS(filepath)));
      allFilenames[i] = PushString(temp,"%s",path.filename().c_str());
    }
  
    String generatedUnitsLocation = GetRelativePathFromSourceToTarget(globalOptions.softwareOutputFilepath,globalOptions.hardwareOutputFilepath,temp);

    TemplateSetString("typeName",typeName);
  
    String simLoopHeader = {};
    if(simulateLoops){
      simLoopHeader = S8("VSuperAddress.h");
    }
    TemplateSetString("simLoopHeader",simLoopHeader);
    TemplateSetString("hardwareFolder",generatedUnitsLocation);
  
    {
      auto s = StartString(temp);
      for(String name : allFilenames){
        s->PushString("$(HARDWARE_FOLDER)/%.*s ",UNPACK_SS(name));
      }

      TemplateSetString("hardwareUnits",EndString(temp,s));
    }

    {
      auto s = StartString(temp);
      for(FUDeclaration* decl : globalDeclarations){
        if(decl->type == FUDeclarationType_COMPOSITE || decl->type == FUDeclarationType_MERGED){
          s->PushString("$(HARDWARE_FOLDER)/%.*s.v ",UN(decl->name));
        }
      }

      TemplateSetString("moduleUnits",EndString(temp,s));
    }

    if(topLevelDecl->memoryMapBits.has_value()){
      TemplateSetNumber("addressSize",topLevelDecl->memoryMapBits.value());
    } else {
      TemplateSetNumber("addressSize",10); // TODO: We need to figure out the best thing to do in this situation.
    }
      
    TemplateSetNumber("databusDataSize",globalOptions.databusDataSize);

    String traceType = {};
    if(globalDebug.outputVCD){
      traceType = S8("--trace");
      if(globalOptions.generateFSTFormat){
        traceType = S8("--trace-fst");
      }

      TemplateSetString("traceType",traceType);
    }

    ProcessTemplateSimple(output,META_MakefileTemplate_Content);
  }
}

void OutputTopLevel(Accelerator* accel,Array<Wire> allStaticsVerilatorSide,AccelInfo info,FUDeclaration* topLevelDecl,Array<TypeStructInfoElement> structuredConfigs,String hardwarePath,VersatComputedValues val,Array<ExternalMemoryInterface> external,Array<WireInformation> wireInfo){
  TEMP_REGION(temp,nullptr);
  // Top accelerator
  FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%.*s/versat_instance.v",UN(hardwarePath)),"w",FilePurpose_VERILOG_CODE);
  DEFER_CLOSE_FILE(s);

  Pool<FUInstance> instances = accel->allocated;
  
  if(!s){
    printf("Error creating file, check if filepath is correct: %.*s\n",UN(hardwarePath));
    return;
  }

  VEmitter* m = StartVCode(temp);

  VerilogInterfaceSpec databus[] = {
    {S8("ready"),{},true},
    {S8("valid"),{}},
    {S8("addr"),S8("AXI_ADDR_W")},
    {S8("rdata"),S8("AXI_DATA_W"),true,true},
    {S8("wdata"),S8("AXI_DATA_W")},
    {S8("wstrb"),S8("(AXI_DATA_W/8)")},
    {S8("len"),S8("LEN_W")},
    {S8("last"),{},true},
  };
    
  Array<VerilogInterfaceSpec> data = {databus,ARRAY_SIZE(databus)};
    
  EmitIOUnpacking(m,val.nUnitsIO,data,S8("databus"),S8("m_databus"));
    
  VAST* ast = EndVCode(m);

  auto b = StartString(temp);
  Repr(ast,b);
  String content = EndString(temp,b);
    
  TemplateSetString("emitIO",content);

  {
    VEmitter* m = StartVCode(temp);
    EmitConnectOutputsToOut(m,instances);
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("assignOuts",content);
  }

  {
    Array<Wire> configs = topLevelDecl->configs;

    VEmitter* m = StartVCode(temp);
    EmitTopLevelInstanciateUnits(m,val,instances,topLevelDecl,configs);
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("instanciateUnits",content);
  }

  {
    String content = EmitExternalMemoryPort(external,temp);
    TemplateSetString("externalMemPorts",content);
  }

  {
    VEmitter* m = StartVCode(temp);

    m->StartPortGroup();
    for(int i = 0; i < topLevelDecl->NumberInputs(); i++){
      m->InputIndexed("in%d",i,"DATA_W");
    }
    for(int i = 0; i < topLevelDecl->NumberOutputs(); i++){
      m->OutputIndexed("out%d",i,"DATA_W");
    }
    m->EndPortGroup();
      
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("inAndOuts",content);
  }

  {
    VEmitter* m = StartVCode(temp);

    if(val.nDones){
      m->Wire("unitDone",val.nDones);
    } else {
      m->WireAndAssignJoinBlock("unitDone","",1);
      m->Expression("1'b1");
      m->EndBlock();
    }

    if(val.unitsMapped){
      m->Wire("unitRdataFinal","DATA_W");
    }
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("wireDecls",content);
  }

  {
    if(globalOptions.useDMA){
      String content = S8(R"FOO(
reg [LEN_W-1:0] dma_length;
reg [ADDR_W-1:0] dma_internal_address_start;
reg [AXI_ADDR_W-1:0] dma_external_addr_start;

wire dma_ready;
wire [ADDR_W-1:0] dma_addr_in;
wire [AXI_DATA_W-1:0] dma_rdata;

SimpleDMA #(.ADDR_W(ADDR_W),.DATA_W(DATA_W),.AXI_ADDR_W(AXI_ADDR_W)) dma (
  .m_databus_ready(databus_ready[`nIO - 1]),
  .m_databus_valid(databus_valid[`nIO - 1]),
  .m_databus_addr(databus_addr[`nIO - 1]),
  .m_databus_rdata(databus_rdata[`nIO - 1]),
  .m_databus_wdata(databus_wdata[`nIO - 1]),
  .m_databus_wstrb(databus_wstrb[`nIO - 1]),
  .m_databus_len(databus_len[`nIO - 1]),
  .m_databus_last(databus_last[`nIO - 1]),

  .addr_internal(dma_internal_address_start),
  .addr_read(dma_external_addr_start),
  .length(dma_length),

  .run(csr_valid && we && csr_addr == 16),
  .running(dma_running),

  .valid(dma_ready),
  .address(dma_addr_in),
  .data(dma_rdata),

  .clk(clk),
  .rst(rst_int)
);

always @(posedge clk,posedge rst_int)
begin
   if(rst_int) begin
      dma_length <= 0;
      dma_internal_address_start <= 0;
      dma_external_addr_start <= 0;
   end else begin
      if(csr_valid && we) begin
         if(csr_addr == 4)
            dma_internal_address_start <= csr_wdata;
         if(csr_addr == 8)
            dma_external_addr_start <= csr_wdata;
         if(csr_addr == 12)
            dma_length <= csr_wdata[LEN_W-1:0];
         // csr_addr == 16 being used to start dma
      end
   end
end

JoinTwoSimple #(.DATA_W(DATA_W),.ADDR_W(ADDR_W)) joiner (
  .m0_valid(dma_ready),
  .m0_data(dma_rdata),
  .m0_addr(dma_addr_in),
  .m0_wstrb(~0),

  .m1_valid(csr_valid),
  .m1_data(csr_wdata),
  .m1_addr(csr_addr),
  .m1_wstrb(csr_wstrb),

  .s_valid(data_valid),
  .s_data(data_data),
  .s_addr(address),
  .s_wstrb(data_wstrb)
);
                           )FOO");
        
      TemplateSetString("dmaInstantiation",content);

      String content2 = S8(R"FOO(
         // DMA Read
         if(csr_addr == 4) begin
            if(csr_wstrb == 0) versat_rvalid <= 1'b1;
            versat_rdata <= {31'h0,!dma_running};
         end
                            )FOO");

      TemplateSetString("dmaRead",content2);
    } else {
      String content = S8(R"FOO(
assign data_valid = csr_valid;
assign address = csr_addr;
assign data_data = csr_wdata;
assign data_wstrb = csr_wstrb;
                           )FOO");
        
      TemplateSetString("dmaInstantiation",content);
      TemplateSetString("dmaRead",{});
    }
  }

  {
    if(val.memoryMappedBytes == 0){
      TemplateSetString("memoryConfigDecisionExpr",S8("1'b0"));
    } else {
      String content = PushString(temp,"address[%d]",val.memoryConfigDecisionBit);
      TemplateSetString("memoryConfigDecisionExpr",content);
    }
  }

  {
    VEmitter* m = StartVCode(temp);
    if(info.unitsMapped >= 1){
      Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

      m->Wire("unitRValid",info.unitsMapped);
      m->Assign("csr_rvalid","versat_rvalid | (|unitRValid)");

      m->Reg("memoryMappedEnable",memoryMasks.size);

      m->WireArray("unitRData",info.unitsMapped,"DATA_W");
      m->WireAndAssignJoinBlock("unitRDataFinal","|","DATA_W");
      for(int i = 0; i < info.unitsMapped; i++){
        m->Expression(SF("unitRData[%d]",i));
      }
      m->EndBlock();
      m->Assign("csr_rdata","versat_rvalid ? versat_rdata : unitRDataFinal");

      m->CombBlock();
      {
        m->Set("memoryMappedEnable",SF("%d'b0",info.unitsMapped));
        m->If("data_valid & memoryMappedAddr");
        for(int i = 0; i <  memoryMasks.size; i++){
          String mask  =  memoryMasks[i];
          if(!Empty(mask)){
            m->If(SF("csr_addr[(%d-1) -: %d] == %d'b%.*s",topLevelDecl->memoryMapBits.value(),mask.size,mask.size,UNPACK_SS(mask)));
            m->Set(SF("memoryMappedEnable[%d]",i),"1'b1");
            m->EndIf();
          } else {
            m->Set(SF("memoryMappedEnable[%d]",i),"1'b1");
          }
        }
        m->EndIf();
      }
      m->EndBlock();
    } else {
      m->Assign("csr_rdata","versat_rdata");
      m->Assign("csr_rvalid","versat_rvalid");
    }

    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("unitsMappedDecl",content);
  }

  {
    VEmitter* m = StartVCode(temp);

    if(info.numberConnections){
      for(int i = 0; i < instances.Size(); i++){
        FUInstance* node = instances.Get(i);
        for(int k = 0; k < node->outputs.size; k++){
          bool out = node->outputs[k];
          if(out){
            m->Wire(SF("output_%d_%d",i,k),"DATA_W");
          }
        }
      }
    }

    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("connections",content);
  }
  {
    VEmitter* m = StartVCode(temp);
      
    if(val.configurationBits){
      String repr = PushRepresentation(val.configurationBitsExpr,temp);
        
      m->Wire("configdata",CS(repr));

      for(auto info : wireInfo){
        if(info.isStatic){
          m->Wire(CS(info.wire.name),info.wire.bitSize);
        }
      }

      m->StartInstance("versat_configurations","configs");

      // TODO: This should be all the global params that Versat cares about. We need to standardize this and properly define all the global parameters
      m->InstanceParam("ADDR_W","ADDR_W");
      m->InstanceParam("DATA_W","DATA_W");
      m->InstanceParam("AXI_DATA_W","AXI_DATA_W");
      m->InstanceParam("AXI_ADDR_W","AXI_ADDR_W");
      m->InstanceParam("DELAY_W","DELAY_W");
      m->InstanceParam("LEN_W","LEN_W");

      m->PortConnect("config_data_o","configdata");
      m->PortConnect("memoryMappedAddr","memoryMappedAddr");
      m->PortConnect("data_write","csr_valid && we");

      m->PortConnect("address","address");
      m->PortConnect("data_wstrb","data_wstrb");
      m->PortConnect("data_data","data_data");

      m->PortConnect("change_config_pulse","pre_run_pulse");

      for(auto info : wireInfo){
        if(info.isStatic){
          m->PortConnect(info.wire.name,info.wire.name);
        }
      }

      m->PortConnect("clk_i","clk");
      m->PortConnect("rst_i","rst_int");
        
      m->EndInstance();
    }

    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("configurationDecl",content);
  }

  {
    VEmitter* m = StartVCode(temp);

    if(val.stateBits){
      m->Wire("statedata",val.stateBits);
    }
      
    m->CombBlock();

    m->Set("stateRead","32'h0");
    m->If("csr_valid & !we & !memoryMappedAddr");

    int stateBitsSeen = 0;
    int addr = val.versatStates;
    for(int i = 0; i < topLevelDecl->states.size; i++){
      Wire wire  = topLevelDecl->states[i];
      m->If(SF("csr_addr[%d:2] == %d",val.stateAddressBits + 1,addr));
      m->Set("stateRead",SF("statedata[%d+:%d]",stateBitsSeen,wire.bitSize));
      stateBitsSeen += wire.bitSize;
      addr += 1;
      m->EndIf();
    }
      
    m->EndIf();
      
    m->EndBlock();
      
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("stateDecl",content);
  }
    
  {
    VEmitter* m = StartVCode(temp);

    EmitCombOperations(m,instances);
      
    auto b = StartString(temp);
    Repr(EndVCode(m),b);
    String content = EndString(temp,b);
      
    TemplateSetString("combOperations",content);
  }

  TemplateSetString("typeName",accel->name);
  TemplateSetNumber("nConfigs",val.nConfigs);
  TemplateSetBool("useDMA",globalOptions.useDMA);
  TemplateSetNumber("databusDataSize",globalOptions.databusDataSize);
    
  ProcessTemplateSimple(s,META_TopInstanceTemplate_Content);
}
  
void OutputHeader(Array<TypeStructInfoElement> structuredConfigs,AccelInfo info,bool isSimple,Accelerator* accel,String softwarePath,Array<Wire> allStaticsVerilatorSide,VersatComputedValues val){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  AccelInfoIterator iter = StartIteration(&info);

  StructInfo* stateStructInfo = GenerateStateStruct(iter,temp);
  Array<TypeStructInfo> stateStructs = {};
  if(!Empty(stateStructInfo->memberList)){
    // We generate an extra level, so we just remove it here.
    stateStructInfo = stateStructInfo->memberList->head->elem.childStruct;

    Array<StructInfo*> allStateStructs = ExtractStructs(stateStructInfo,temp);

    // TODO: Maybe turn into a function together with the code below
    Array<int> indexes = PushArray<int>(temp,allStateStructs.size);
    Memset(indexes,2);
    for(int i = 0; i < allStateStructs.size; i++){
      String name = allStateStructs[i]->name;
      
      for(int ii = 0; ii < i; ii++){
        String possibleDuplicate = allStateStructs[ii]->name;

        // TODO: The fact that we are doing string comparison is kinda bad.
        if(CompareString(possibleDuplicate,name)){
          allStateStructs[i]->name = PushString(temp,"%.*s_%d",UNPACK_SS(possibleDuplicate),indexes[ii]++);
          break;
        }
      }
    }

    stateStructs = GenerateStructs(allStateStructs,STRING("State"),false,temp);
  }

  // NOTE: This function also fills the instance info member of the acceleratorInfo. This function only fills the first partition, but I think that it is fine because that is the only one we use. We generate the same structs either way.
  StructInfo* structInfo = GenerateConfigStruct(iter,temp);
  Array<TypeStructInfo> structs = {};
  // If we only contain static configs, this will appear empty.
  if(!Empty(structInfo->memberList)){
    // We generate an extra level, so we just remove it here.
    structInfo = structInfo->memberList->head->elem.childStruct;
  
    // NOTE: We for now push all the merge muxs to the top.
    //       It might be better to only push them to the merge struct (basically only 1 level up).
    //       Still need more examples to see.
    PushMergeMultiplexersUpTheHierarchy(structInfo);
    
    Array<StructInfo*> allStructs = ExtractStructs(structInfo,temp);
    
    Array<int> indexes = PushArray<int>(temp,allStructs.size);
    Memset(indexes,2);
    for(int i = 0; i < allStructs.size; i++){
      String name = allStructs[i]->name;
      
      for(int ii = 0; ii < i; ii++){
        String possibleDuplicate = allStructs[ii]->name;

        // TODO: The fact that we are doing string comparison is kinda bad.
        if(CompareString(possibleDuplicate,name)){
          allStructs[i]->name = PushString(temp,"%.*s_%d",UNPACK_SS(possibleDuplicate),indexes[ii]++);
          break;
        }
      }
    }

    structs = GenerateStructs(allStructs,STRING("Config"),true,temp);
  }
  
  TemplateSetBool("isSimple",isSimple);
  FUInstance* simpleInstance = nullptr;
  if(isSimple){
    FUInstance* inst = nullptr; // TODO: Should probably separate isSimple to a separate function, because otherwise we are recalculating stuff that we already know.
    for(FUInstance* ptr : accel->allocated){
      if(CompareString(ptr->name,STRING("TOP"))){
        inst = ptr;
        break;
      }
    }
    Assert(inst);

    Accelerator* accel = inst->declaration->fixedDelayCircuit;
    if(accel){
      for(FUInstance* ptr : accel->allocated){
        if(CompareString(ptr->name,STRING("simple"))){
          inst = ptr;
          break;
        }
      }
      Assert(inst);

      simpleInstance = inst;
      
      TemplateSetNumber("simpleInputs",inst->declaration->NumberInputs());
      TemplateSetNumber("simpleOutputs",inst->declaration->NumberOutputs());
    } else {
      TemplateSetNumber("simpleInputs",0);
      TemplateSetNumber("simpleOutputs",0);
    }
  }

  // Accelerator header
  auto arr = StartArray<int>(temp);
  for(InstanceInfo& t : info.infos[0].info){
    if(!t.isComposite){
      for(int d : t.extraDelay){
        *arr.PushElem() = d;
      }
    }
  }
  Array<int> delays = EndArray(arr);

  // TODO: We eventually only want to put this as true if we output at least one address gen.
  TemplateSetBool("simulateLoops",true);

  {
    CEmitter* m = StartCCode(temp);
    m->Struct(STRING("AddressVArguments"));
    for(String str : META_AddressVParameters_Members){
      m->Member(STRING("iptr"),str);
    }
    m->EndBlock();

    m->Struct(STRING("AddressGenArguments"));
    for(String str : META_AddressGenParameters_Members){
      m->Member(STRING("iptr"),str);
    }
    m->EndBlock();

    m->Struct(STRING("AddressMemArguments"));
    for(String str : META_AddressMemParameters_Members){
      m->Member(STRING("iptr"),str);
    }
    m->EndBlock();
    
    CAST* ast = EndCCode(m);
    auto b = StartString(temp);
    Repr(ast,b);
    TemplateSetString("AddressStruct",EndString(temp,b));
  }
    
  Array<Array<int>> allDelays = PushArray<Array<int>>(temp,info.infos.size);
  if(info.infos.size >= 2){
    int i = 0;
    for(int ii = 0; ii <  info.infos.size; ii++){
      Array<InstanceInfo> allInfos = info.infos[ii].info;
      auto arr = StartArray<int>(temp);
      for(InstanceInfo& t : allInfos){
        if(!t.isComposite){
          for(int d : t.extraDelay){
            *arr.PushElem() = d;
          }
        }
      }
      Array<int> delays = EndArray(arr);
      allDelays[i++] = delays;
    }
  }
  TemplateSetNumber("amountMerged",allDelays.size);

  auto diffArray = StartArray<DifferenceArray>(temp2);
  for(int oldIndex = 0; oldIndex < allDelays.size; oldIndex++){
    for(int newIndex = 0; newIndex < allDelays.size; newIndex++){
      if(oldIndex == newIndex){
        continue;
      }
        
      Array<Difference> difference = CalculateSmallestDifference(allDelays[oldIndex],allDelays[newIndex],temp);

      DifferenceArray* diff = diffArray.PushElem();
      diff->oldIndex = oldIndex;
      diff->newIndex = newIndex;
      diff->differences = difference;
    }
  }

  Array<String> allStates = ExtractStates(info.infos[0].info,temp2);
  Array<Pair<String,int>> allMem = ExtractMem(info.infos[0].info,temp2);

  TemplateSetBool("outputChangeDelay",false);

  Array<String> names = Extract(info.infos,temp,&MergePartition::name);
  Array<Array<MuxInfo>> muxInfo = CalculateMuxInformation(&iter,temp);

  // Combines address gen with the struct names that use them. Mostly done this way because we generate C code which does not support method overloading meaning that we have to do it manually.
  TrieSet<Pair<String,AccessAndType>>* structNameAndAddressGen = PushTrieSet<Pair<String,AccessAndType>>(temp);
  TrieSet<AccessAndType>* addressGenUsed = PushTrieSet<AccessAndType>(temp);
  TrieSet<Pair<String,AddressGenType>>* structsUsed = PushTrieSet<Pair<String,AddressGenType>>(temp);

  AccelInfoIterator top = StartIteration(&info);
  for(int i = 0; i < top.MergeSize(); i++){
    AccelInfoIterator iter = top;
    iter.SetMergeIndex(i);
      
    for(;iter.IsValid(); iter = iter.Step()){
      InstanceInfo* unit = iter.CurrentUnit();

      for(String addressGenName : unit->addressGenUsed){
        AddressAccess* access = GetAddressGenOrFail(addressGenName);

        Assert(unit->structInfo); // If using address gen, need to have a valid struct info otherwise cannot proceed
        Assert(!Empty(unit->structInfo->name));
        Assert(!Empty(unit->structInfo->originalName));
        
        String typeName = unit->structInfo->name;

        // TODO: We need to do this check beforehand. At this point we should already guarantee that everything is possible.
        if((int) unit->decl->supportedAddressGenType == 0){
          printf("[USER_ERROR] Unit '%.*s' does not support address gen\n",UNPACK_SS(unit->decl->name));
          exit(-1);
        }

        AddressGenType type = unit->decl->supportedAddressGenType;
        structNameAndAddressGen->Insert({typeName,{access,type}});
        addressGenUsed->Insert({access,type});
        structsUsed->Insert({typeName,type});
      }
    }
  }
    
  // By getting the info directly from the structs, we make sure that we are always generating the functions needed for each possible structure that gets created/changed due to partial share or merge.
  // Address gen is therefore capable of handling all the struct modifing features as long as they are properly implemented inside structInfo.

  GrowableArray<String> builder = StartArray<String>(temp2);
    
  for(AccessAndType p : addressGenUsed){
    AddressAccess* initial = p.access;
    AddressGenType type = p.type;
    String data = GenerateAddressGenCompilationFunction(initial,type,temp);
    *builder.PushElem() = data;
  }

  for(Pair<String,AddressGenType> p : structsUsed){
    String structName = p.first;
    AddressGenType type = p.second;

    String content = GenerateAddressLoadingFunction(structName,type,temp);
    *builder.PushElem() = content;
  }

  for(Pair<String,AccessAndType> p : structNameAndAddressGen){
    String structName = p.first;
    AddressAccess* initial = p.second.access;
    AddressGenType type = p.second.type;

    String content = GenerateAddressCompileAndLoadFunction(structName,initial,type,temp);
    *builder.PushElem() = content;
  }

  Array<String> content = EndArray(builder);

  auto b = StartString(temp);

  for(String s : content){
    b->PushString(s);
    b->PushString("\n");
  }

  TemplateSetString("allAddrGen",EndString(temp,b));
    
  FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%.*s/versat_accel.h",UN(softwarePath)),"w",FilePurpose_SOFTWARE);
  DEFER_CLOSE_FILE(f);

  {
    CEmitter* c = StartCCode(temp);

    if(structs.size == 0){
      String typeName = accel->name;
      c->Struct(PushString(temp,"%.*sConfig",UN(typeName)));
      c->EndStruct();
    }
      
    for(TypeStructInfo info : structs){
      c->Define(PushString(temp,"VERSAT_DEFINED_%.*s",UN(info.name)));
      c->Struct(PushString(temp,"%.*sConfig",UN(info.name)));
      for(TypeStructInfoElement entry : info.entries){
        if(entry.typeAndNames.size > 1){
          c->Union();
          
          for(SingleTypeStructElement typeAndName : entry.typeAndNames){
            if(typeAndName.arraySize > 1){
              c->Member(typeAndName.type,typeAndName.name,typeAndName.arraySize);
            } else {
              c->Member(typeAndName.type,typeAndName.name);
            }
          }

          c->EndStruct();
        } else {
          SingleTypeStructElement elem = entry.typeAndNames[0];
          c->Member(elem.type,elem.name,elem.arraySize);
        }
      }

      c->EndStruct();
      c->Line();
    }

    String content = PushASTRepr(c,temp);
    TemplateSetString("configStructs",content);
  }

  String typeName = accel->name;
  {
    CEmitter* c = StartCCode(temp);

    if(stateStructs.size == 0){
      c->Struct(PushString(temp,"%.*sState",UN(typeName)));
      c->EndStruct();
    }
      
    for(TypeStructInfo info : stateStructs){
      c->Struct(PushString(temp,"%.*sState",UN(info.name)));
      for(TypeStructInfoElement entry : info.entries){
        if(entry.typeAndNames.size > 1){
          c->Union();
          
          for(SingleTypeStructElement typeAndName : entry.typeAndNames){
            if(typeAndName.arraySize > 1){
              c->Member(typeAndName.type,typeAndName.name,typeAndName.arraySize);
            } else {
              c->Member(typeAndName.type,typeAndName.name);
            }
          }

          c->EndStruct();
        } else {
          SingleTypeStructElement elem = entry.typeAndNames[0];
          c->Member(elem.type,elem.name,elem.arraySize);
        }
      }

      c->EndStruct();
      c->Line();
    }

    String content = PushASTRepr(c,temp);
    TemplateSetString("stateStructs",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);

    c->Struct(S8("AcceleratorState"));
    for(String name : allStates){
      c->Member(S8("int"),name);
    }
    c->EndStruct();

    String content = PushASTRepr(c,temp);
    TemplateSetString("acceleratorState",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    Array<TypeStructInfo> addressStructures = GetMemMappedStructInfo(&info,temp2);

    for(TypeStructInfo info : addressStructures){
      c->Define(PushString(temp,"VERSAT_DEFINED_%.*sAddr",UN(info.name)));
      c->Struct(PushString(temp,"%.*sAddr",UN(info.name)));

      for(TypeStructInfoElement elem : info.entries){
        SingleTypeStructElement single = elem.typeAndNames[0];
        c->Member(single.type,single.name);
      }
        
      c->EndStruct();
    }

    String content = PushASTRepr(c,temp);
    TemplateSetString("addrStructs",content);
  }

  {
    CEmitter* c = StartCCode(temp);
    c->Struct(S8("AcceleratorConfig"));

    for(auto elem : structuredConfigs){
      if(elem.typeAndNames.size > 1){
        c->Union();

        for(auto typeAndName : elem.typeAndNames){
          c->Member(typeAndName.type,typeAndName.name);
        }
          
        c->EndStruct();
      } else {
        c->Member(elem.typeAndNames[0].type,elem.typeAndNames[0].name);
      }
    }
      
    c->EndStruct();
    String content = PushASTRepr(c,temp);
    TemplateSetString("acceleratorConfig",content);
  }

  {
    CEmitter* c = StartCCode(temp);
    c->Struct(S8("AcceleratorStatic"));

    for(auto elem : allStaticsVerilatorSide){
      c->Member(S8("iptr"),elem.name);
    }
      
    c->EndStruct();
    String content = PushASTRepr(c,temp);
    TemplateSetString("acceleratorStatic",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);
    c->Struct(S8("AcceleratorDelay"));
    {
      c->Union();
      {
        c->Struct();
        for(int i = 0 ;i < info.delays; i++){
          c->Member(S8("iptr"),PushString(temp,"TOP_Delay%d",i));
        }
        c->EndStruct();

        c->Member(S8("iptr"),S8("delays"),info.delays);
      }
      c->EndStruct();
    }
    c->EndStruct();
    String content = PushASTRepr(c,temp);
    TemplateSetString("acceleratorDelay",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);

    for(Pair<String,int> p : allMem){
      c->Define(p.first,PushString(temp,"((void*) (versat_base + memMappedStart + 0x%x))",p.second));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("memMappedAddresses",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    c->VarBlock();
    for(Pair<String,int> p : allMem){
      c->Elem(p.first);
    }
    c->EndBlock();

    String content = PushASTRepr(c,temp);
    TemplateSetString("addrBlock",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    c->VarBlock();
    for(auto d : delays){
      c->Elem(PushString(temp,"0x%x",d));
    }
    c->EndBlock();

    String content = PushASTRepr(c,temp);
    TemplateSetString("delayBlock",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    if(stateStructs.size > 0){
      c->RawLine(PushString(temp,"extern volatile %.*sState* accelState;",UN(typeName)));
    } else {
      c->RawLine(S8("extern volatile AcceleratorState* accelState;"));
    };

    String content = PushASTRepr(c,temp);
    TemplateSetString("accelStateDecl",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    for(auto elem : allStaticsVerilatorSide){
      c->Define(PushString(temp,"ACCEL_%.*s",UN(elem.name)),PushString(temp,"accelStatic->%.*s",UN(elem.name)));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("allStaticDefines",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);

    if(isSimple){
      c->Define(S8("NumberSimpleInputs"),PushString(temp,"%d",simpleInstance->declaration->NumberInputs()));
      c->Define(S8("NumberSimpleOutputs"),PushString(temp,"%d",simpleInstance->declaration->NumberOutputs()));
      c->Define(S8("SimpleInputStart"),S8("((iptr*) accelConfig)"));
      c->Define(S8("SimpleOutputStart"),S8("((int*) accelState)"));
    }
        
    String content = PushASTRepr(c,temp);
    TemplateSetString("simpleStuff",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    if(names.size > 1){
      for(int i = 0; i <  allDelays.size; i++){
        auto delayArray  =  allDelays[i];
        c->ArrayDeclareBlock("unsigned int",SF("delayBuffer_%d",i),true);

        for(auto d : delayArray){
          c->Elem(PushString(temp,"0x%x",d));
        }

        c->EndBlock();
      }

      c->ArrayDeclareBlock("unsigned int*","delayBuffers",true);

      for(int i = 0; i < names.size; i++){
        c->Elem(PushString(temp,"delayBuffer_%d",i));
      }
        
      c->EndBlock();

      if(muxInfo.size > 0){
        c->Enum(S8("MergeType"));
        for(int i = 0; i <  names.size; i++){
          auto name  =  names[i];
          c->EnumMember(PushString(temp,"MergeType_%.*s",UN(name)),PushString(temp,"%d",i));
        }
        c->EndEnum();

        c->FunctionBlock(S8("static inline void"),S8("ActivateMergedAccelerator"));
        c->Argument(S8("MergeType"),S8("type"));

        c->Assignment(S8("int asInt"),S8("(int) type"));

        c->SwitchBlock(S8("type"));
        for(int i = 0 ; i < names.size; i++){
          c->CaseBlock(PushString(temp,"MergeType_%.*s",UN(names[i])));

          for(auto info : muxInfo[i]){
            c->Assignment(PushString(temp,"accelConfig->%.*s.sel",UN(info.name)),PushString(temp,"%d",info.val));
          }
          c->EndBlock();
        }
        c->EndBlock();
        
        c->RawLine(S8("VersatLoadDelay(delayBuffers[asInt]);"));
      
        c->EndBlock();
      }
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("mergeStuff",content);
  }

  TemplateSetNumber("databusDataSize",globalOptions.databusDataSize);
  TemplateSetHex("memMappedStart",1 << val.memoryConfigDecisionBit);
  TemplateSetHex("versatAddressSpace",2 * (1 << val.memoryConfigDecisionBit));
  TemplateSetHex("staticStart",val.nConfigs);
  TemplateSetHex("delayStart",val.nConfigs + val.nStatics);
  TemplateSetHex("configStart",val.versatConfigs);
  TemplateSetHex("stateStart",val.versatStates);
  TemplateSetBool("useDMA",globalOptions.useDMA);
  TemplateSetString("typeName",accel->name);
  TemplateSetNumber("nConfigs",val.nConfigs);

  ProcessTemplateSimple(f,META_HeaderTemplate_Content);
}

void OutputVerilatorWrapper(Accelerator* accel,Array<Wire> allStaticsVerilatorSide,AccelInfo info,FUDeclaration* topLevelDecl,Array<TypeStructInfoElement> structuredConfigs,String softwarePath){
  TEMP_REGION(temp,nullptr);
  Pool<FUInstance> nodes = accel->allocated;

  // We need to bundle config + static (type->config) only contains config, but not static
  // TODO: This is not good. Eventually need to take a look at what is happening inside the wrapper.
  auto arr = StartArray<WireExtra>(temp);
  for(auto n : nodes){
    for(Wire config : n->declaration->configs){
      WireExtra* ptr = arr.PushElem();
      *ptr = config;
      ptr->source = STRING("config->TOP_");
      ptr->source2 = STRING("config->");
    }
  }
  for(Wire& staticWire : allStaticsVerilatorSide){
    WireExtra* ptr = arr.PushElem();
    *ptr = staticWire;
    ptr->source = STRING("statics->");
    ptr->source2 = STRING("statics->");
  }
  Array<WireExtra> allConfigsVerilatorSide = EndArray(arr);

  auto builder = StartArray<ExternalMemoryInterface>(temp);
  for(AccelInfoIterator iter = StartIteration(&info); iter.IsValid(); iter = iter.Next()){
    for(ExternalMemoryInterface& inter : iter.CurrentUnit()->decl->externalMemory){
      *builder.PushElem() = inter;
    }
  }
  auto external = EndArray(builder);

  TemplateSetNumber("delays",info.delays);
  Array<String> statesHeaderSide = ExtractStates(info.infos[0].info,temp);
  
  TemplateSetNumber("nInputs",info.inputs);
  TemplateSetBool("implementsDone",info.implementsDone);

  TemplateSetNumber("memoryMapBits",info.memoryMappedBits);
  TemplateSetNumber("nIOs",info.nIOs);
  TemplateSetBool("trace",globalDebug.outputVCD);
  TemplateSetBool("signalLoop",info.signalLoop);
  TemplateSetNumber("numberDelays",info.delays);

  {
    CEmitter* c = StartCCode(temp);

    if(globalDebug.outputVCD){
      c->Define(S8("TRACE"));
    }
      
    if(globalOptions.generateFSTFormat){
      c->Define(S8("TRACE_FST"));
    } else {
      c->Define(S8("TRACE_VCD"));
    }

    if(info.signalLoop){
      c->Define(S8("SIGNAL_LOOP"));
    }

    if(true){
      c->Define(S8("SIMULATE_LOOPS"));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("defines",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    for(auto ext : external){
      int id = ext.interface;

      // NOTE: This defines work from values that are extracted directly from the header generated by verilator. This ensures that we match the actual expected size since the accelerator needs to be instantiated at generation time.
      FULL_SWITCH(ext.type){
      case ExternalMemoryType::DP:{
        c->Comment(S8("DP"));

        c->Define(PushString(temp,"WIRE_SIZE_ext_dp_addr_%d",id),
                  PushString(temp,"(UNIT_MSB_ext_dp_addr_%d_port_0 - UNIT_LSB_ext_dp_addr_%d_port_0 + 1)",id,id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_dp_%d",id),
                  PushString(temp,"(1 << (WIRE_SIZE_ext_dp_addr_%d))",id));
          
        c->Define(PushString(temp,"WIRE_SIZE_ext_dp_data_%d_0",id),
                  PushString(temp,"(UNIT_MSB_ext_dp_out_%d_port_0 - UNIT_LSB_ext_dp_out_%d_port_0 + 1)",id,id));
          
        c->Define(PushString(temp,"WIRE_SIZE_ext_dp_data_%d_1",id),
                  PushString(temp,"(UNIT_MSB_ext_dp_out_%d_port_1 - UNIT_LSB_ext_dp_out_%d_port_1 + 1)",id,id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_dp_data_%d_0",id),
                  PushString(temp,"(WIRE_SIZE_ext_dp_data_%d_0 / 8)",id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_dp_data_%d_1",id),
                  PushString(temp,"(WIRE_SIZE_ext_dp_data_%d_1 / 8)",id));
      } break;
      case ExternalMemoryType::TWO_P:{
        c->Comment(S8("2P"));

        c->Define(PushString(temp,"WIRE_SIZE_ext_2p_addr_in_%d",id),
                  PushString(temp,"(UNIT_MSB_ext_2p_addr_in_%d - UNIT_LSB_ext_2p_addr_in_%d + 1)",id,id));

        c->Define(PushString(temp,"WIRE_SIZE_ext_2p_addr_out_%d",id),
                  PushString(temp,"(UNIT_MSB_ext_2p_addr_out_%d - UNIT_LSB_ext_2p_addr_out_%d + 1)",id,id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_2p_%d",id),
                  PushString(temp,"(1 << (WIRE_SIZE_ext_2p_addr_in_%d))",id));

        c->Define(PushString(temp,"WIRE_SIZE_ext_2p_data_in_%d",id),
                  PushString(temp,"(UNIT_MSB_ext_2p_data_in_%d - UNIT_LSB_ext_2p_data_in_%d + 1)",id,id));

        c->Define(PushString(temp,"WIRE_SIZE_ext_2p_data_out_%d",id),
                  PushString(temp,"(UNIT_MSB_ext_2p_data_out_%d - UNIT_LSB_ext_2p_data_out_%d + 1)",id,id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_2p_data_in_%d",id),
                  PushString(temp,"(WIRE_SIZE_ext_2p_data_in_%d / 8)",id));

        c->Define(PushString(temp,"BYTE_SIZE_ext_2p_data_out_%d",id),
                  PushString(temp,"(WIRE_SIZE_ext_2p_data_out_%d / 8)",id));
      } break;
    } END_SWITCH();
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("memoryWireInfo",content);
  }
  {
    CEmitter* c = StartCCode(temp);

    auto* b = StartString(temp);
    for(int i = 0; i <  external.size; i++){
      auto ext  =  external[i];
      int id = ext.interface;
      if(i != 0){
        b->PushString(" + ");
      }
        
      FULL_SWITCH(ext.type){
      case ExternalMemoryType::DP:{
        b->PushString("BYTE_SIZE_ext_dp_%d",id);
      } break;
      case ExternalMemoryType::TWO_P:{
        b->PushString("BYTE_SIZE_ext_2p_%d",id);
      } break;
    } END_SWITCH();
    }
    String sum = EndString(temp,b);

    if(external.size == 0){
      sum = S8("0");
    }
      
    c->VarDeclare(S8("static constexpr int"),S8("totalExternalMemory"),sum);
    c->VarDeclare(S8("static Byte"),S8("externalMemory[totalExternalMemory]"));
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("declareExternalMemory",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    for(int i = 0; i < info.delays; i++){
      c->Assignment(PushString(temp,"self->delay%d",i),PushString(temp,"delayBuffer[%d]",i));
    }
    c->RawLine(S8("self->eval();"));
        
    String content = PushASTRepr(c,temp,false,1);
    TemplateSetString("setDelays",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    for(int i = 0; i < info.inputs; i++){
      c->Assignment(PushString(temp,"self->in%d",i),S8("0"));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("inStart",content);
  }

  {
    String simulateBusTemplate = S8(R"FOO(
if(SimulateDatabus){
   self->databus_ready_@{i} = 0;
   self->databus_last_@{i} = 0;

   DatabusAccess* access = &databusBuffer[@{i}];

   if(self->databus_valid_@{i}){
      if(access->latencyCounter > 0){
         access->latencyCounter -= 1;
      } else {
         char* ptr = (char*) (self->databus_addr_@{i});

         if(self->databus_wstrb_@{i} == 0){
            if(ptr == nullptr){
              memset(&self->databus_rdata_@{i},0xdf,sizeOfData);
            } else {
              memcpy(&self->databus_rdata_@{i},&ptr[access->counter * sizeOfData],sizeOfData);
            }
         } else { // self->databus_wstrb_@{i} != 0
            if(ptr != nullptr){
              memcpy(&ptr[access->counter * sizeOfData],&self->databus_wdata_@{i},sizeOfData);
            }
         }
         self->databus_ready_@{i} = 1;

         int transferLength = self->databus_len_@{i};
         int countersLength = ALIGN_UP(transferLength,sizeOfData) / sizeOfData;

         if(access->counter >= countersLength - 1){
            access->counter = 0;
            self->databus_last_@{i} = 1;
         } else {
            access->counter += 1;
         }

         access->latencyCounter = MEMORY_LATENCY;
      }
   }

   self->eval();
}
                                     )FOO""");

    auto* s = StartString(temp);
    for(int i = 0; i < info.nIOs; i++){
      Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
      vals->Insert(S8("i"),PushString(temp,"%d",i));
      TemplateSimpleSubstitute(s,simulateBusTemplate,vals);
    }
    String content = EndString(temp,s);
      
    TemplateSetString("databusSim",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);

    c->Comment(S8("Save external data before updating the rest of the simulation"));
      
    String saveExternalDP = S8(R"FOO(
   int saved_dp_enable_@{id}_port_0 = self->ext_dp_enable_@{id}_port_0;
   int saved_dp_write_@{id}_port_0 = self->ext_dp_write_@{id}_port_0;
   int saved_dp_addr_@{id}_port_0 = self->ext_dp_addr_@{id}_port_0;

   int saved_dp_enable_@{id}_port_1 = self->ext_dp_enable_@{id}_port_1;
   int saved_dp_write_@{id}_port_1 = self->ext_dp_write_@{id}_port_1;
   int saved_dp_addr_@{id}_port_1 = self->ext_dp_addr_@{id}_port_1;

   uint8_t saved_dp_data_@{id}_port_0[BYTE_SIZE_ext_dp_data_@{id}_0]; 
   memcpy(saved_dp_data_@{id}_port_0,&self->ext_dp_out_@{id}_port_0,BYTE_SIZE_ext_dp_data_@{id}_0);

   uint8_t saved_dp_data_@{id}_port_1[BYTE_SIZE_ext_dp_data_@{id}_1]; 
   memcpy(saved_dp_data_@{id}_port_1,&self->ext_dp_out_@{id}_port_1,BYTE_SIZE_ext_dp_data_@{id}_1);

   baseAddress += BYTE_SIZE_ext_dp_@{id};
                                )FOO");

    String saveExternalTwoP = S8(R"FOO(
   uint8_t saved_2p_r_data_@{id}[BYTE_SIZE_ext_2p_data_in_@{id}];
   if(self->ext_2p_read_@{id}){
     int readOffset = self->ext_2p_addr_in_@{id};
     CopyFromMemory(saved_2p_r_data_@{id},&externalMemory[baseAddress],readOffset,BYTE_SIZE_ext_2p_data_in_@{id});
   }
   baseAddress += BYTE_SIZE_ext_2p_@{id};

   int saved_2p_r_enable_@{id} = self->ext_2p_read_@{id};
   int saved_2p_r_addr_@{id} = self->ext_2p_addr_in_@{id}; // Instead of saving address, should access memory and save data. Would simulate better what is actually happening
   
   int saved_2p_w_enable_@{id} = self->ext_2p_write_@{id};
   int saved_2p_w_addr_@{id} = self->ext_2p_addr_out_@{id};

   uint8_t saved_2p_w_data_@{id}[BYTE_SIZE_ext_2p_data_out_@{id}];
   memcpy(saved_2p_w_data_@{id},&self->ext_2p_data_out_@{id},BYTE_SIZE_ext_2p_data_out_@{id});
                                  )FOO");
      
    auto* s = StartString(temp);
    for(auto ext : external){
      FULL_SWITCH(ext.type){
      case ExternalMemoryType::DP:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,saveExternalDP,vals);
      } break;
      case ExternalMemoryType::TWO_P:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,saveExternalTwoP,vals);
      } break;
    } END_SWITCH();
    }
    String content = EndString(temp,s);
      
    TemplateSetString("saveExternal",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    c->Comment(S8("Read memory"));
      
    String readMemoryDP = S8(R"FOO(
   // DP
   if(saved_dp_enable_@{id}_port_0 && !saved_dp_write_@{id}_port_0){
     int readOffset = saved_dp_addr_@{id}_port_0; // Byte space

     CopyFromMemory(&self->ext_dp_in_@{id}_port_0,&externalMemory[baseAddress],readOffset,BYTE_SIZE_ext_dp_data_@{id}_0);
   }
   if(saved_dp_enable_@{id}_port_1 && !saved_dp_write_@{id}_port_1){
     int readOffset = saved_dp_addr_@{id}_port_1; // Byte space

     CopyFromMemory(&self->ext_dp_in_@{id}_port_1,&externalMemory[baseAddress],readOffset,BYTE_SIZE_ext_dp_data_@{id}_1);
   }
   baseAddress += BYTE_SIZE_ext_dp_@{id};
                              )FOO");

    String readMemoryTwoP = S8(R"FOO(
   // 2P
   if(saved_2p_r_enable_@{id}){
     int readOffset = saved_2p_r_addr_@{id};
     CopyFromMemory(&self->ext_2p_data_in_@{id},&externalMemory[baseAddress],readOffset,BYTE_SIZE_ext_2p_data_in_@{id});
   }
   baseAddress += BYTE_SIZE_ext_2p_@{id};
                                )FOO");
      
    auto* s = StartString(temp);
    for(auto ext : external){
      FULL_SWITCH(ext.type){
      case ExternalMemoryType::DP:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,readMemoryDP,vals);
      } break;
      case ExternalMemoryType::TWO_P:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,readMemoryTwoP,vals);
      } break;
    } END_SWITCH();
    }
    String content = EndString(temp,s);
      
    TemplateSetString("memoryRead",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);

    c->Comment(S8("Write memory"));
      
    String writeMemoryDP = S8(R"FOO(
   // DP
   if(saved_dp_enable_@{id}_port_0 && saved_dp_write_@{id}_port_0){
     int writeOffset = saved_dp_addr_@{id}_port_0;

     CopyToMemory(&saved_dp_data_@{id}_port_0,&externalMemory[baseAddress],writeOffset,BYTE_SIZE_ext_dp_data_@{id}_0);
   }
   if(saved_dp_enable_@{id}_port_1 && saved_dp_write_@{id}_port_1){
     int writeOffset = saved_dp_addr_@{id}_port_1;

     CopyToMemory(&saved_dp_data_@{id}_port_1,&externalMemory[baseAddress],writeOffset,BYTE_SIZE_ext_dp_data_@{id}_1);
   }
   baseAddress += BYTE_SIZE_ext_dp_@{id};
                               )FOO");

    String writeMemoryTwoP = S8(R"FOO(
   // 2P
   if(saved_2p_w_enable_@{id}){
     int writeOffset = saved_2p_w_addr_@{id};
     CopyToMemory(saved_2p_w_data_@{id},&externalMemory[baseAddress],writeOffset,BYTE_SIZE_ext_2p_data_out_@{id});
   }
   baseAddress += BYTE_SIZE_ext_2p_@{id};
                                 )FOO");
      
    auto* s = StartString(temp);
    for(auto ext : external){
      FULL_SWITCH(ext.type){
      case ExternalMemoryType::DP:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,writeMemoryDP,vals);
      } break;
      case ExternalMemoryType::TWO_P:{
        Hashmap<String,String>* vals = PushHashmap<String,String>(temp,1);
        vals->Insert(S8("id"),PushString(temp,"%d",ext.interface));
        TemplateSimpleSubstitute(s,writeMemoryTwoP,vals);
      } break;
    } END_SWITCH();
    }
    String content = EndString(temp,s);
      
    TemplateSetString("memoryWrite",content);
  }
    
  {
    CEmitter* c = StartCCode(temp);
    c->Comment(S8("Extract state from model"));
      
    if(topLevelDecl->states.size){
      c->RawLine(S8("AcceleratorState* state = &stateBuffer;"));
    }

    for(int i = 0; i < topLevelDecl->states.size; i++){
      Wire w = topLevelDecl->states[i];
        
      c->Assignment(PushString(temp,"state->%.*s",UN(statesHeaderSide[i])),PushString(temp,"self->%.*s",UN(w.name)));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("saveState",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    if(allConfigsVerilatorSide.size){
      c->RawLine(S8(R"FOO(
  AcceleratorConfig* config = (AcceleratorConfig*) &configBuffer;
  AcceleratorStatic* statics = (AcceleratorStatic*) &staticBuffer;
                     )FOO"));

      for(auto wire : allConfigsVerilatorSide){
        // TODO: Need to fix this. Broken after the change to the parameters
#if 0
        if(wire.bitSize != 64){
          String format = S8(R"FOO(
  once{
     if((long long int) @{1}@{0} >= ((long long int) 1 << @{2})){
      printf("[Once] Warning, configuration value contains more bits\n");
      printf("set than the hardware unit is capable of handling\n");
      printf("Name: %s, BitSize: %d, Value: 0x%lx\n","@{0}",@{2},@{1}@{0});
    }
  };
                              )FOO");

          String values[3] = {wire.name,wire.source,PushString(temp,"%d",wire.bitSize)};
            
          c->RawLine(TemplateSubstitute(format,values,temp));
        }
#endif
          
        if(wire.stage == VersatStage_READ){
          c->Assignment(PushString(temp,"self->%.*s",UN(wire.name)),PushString(temp,"%.*s%.*s",UN(wire.source),UN(wire.name)));
        }

        if(wire.stage == VersatStage_COMPUTE){
          String format = S8(R"FOO(
  static iptr COMPUTED_@{0} = 0;
  self->@{0} = COMPUTED_@{0};
  COMPUTED_@{0} = @{1}@{0};
                              )FOO");
          String values[2] = {wire.name,wire.source};
            
          c->RawLine(TemplateSubstitute(format,values,temp));
        }
          
        if(wire.stage == VersatStage_WRITE){
          String format = S8(R"FOO(
  static iptr COMPUTED_@{0} = 0;
  static iptr WRITE_@{0} = 0;

  self->@{0} = COMPUTED_@{0};
  COMPUTED_@{0} = WRITE_@{0};
  WRITE_@{0} = @{1}@{0};
                              )FOO");
          String values[2] = {wire.name,wire.source};
            
          c->RawLine(TemplateSubstitute(format,values,temp));
        }
      }
    }

    for(int i = 0; i < info.nIOs; i++){
      c->RawLine(PushString(temp,"databusBuffer[%d].latencyCounter = INITIAL_MEMORY_LATENCY;\n",i));
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("internalStart",content);
  }

  {
    CEmitter* c = StartCCode(temp);

    if(topLevelDecl->memoryMapBits.has_value()){
      c->Define(S8("HAS_MEMORY_MAP"));

      if(topLevelDecl->memoryMapBits.value() != 0){
        c->Define(S8("MEMORY_MAP_BITS"),PushString(temp,"%d",topLevelDecl->memoryMapBits.value()));
      }
    }
      
    String content = PushASTRepr(c,temp);
    TemplateSetString("memoryAccessDefines",content);
  }
    
  String typeName = accel->name;
  TemplateSetString("typeName",typeName);
    
  if(info.implementsDone){
    TemplateSetString("done",S8("self->done"));
  } else {
    TemplateSetString("done",S8("true"));
  }

  TemplateSetNumber("databusDataSize",globalOptions.databusDataSize);

  String wrapperPath = PushString(temp,"%.*s/wrapper.cpp",UN(softwarePath));
  FILE* output = OpenFileAndCreateDirectories(wrapperPath,"w",FilePurpose_SOFTWARE);
  DEFER_CLOSE_FILE(output);

  if(1){
    AccelInfoIterator iter = StartIteration(&info);
    Array<Array<MuxInfo>> muxInfo = CalculateMuxInformation(&iter,temp);

    // From accel config, obtain the merge index.
    CEmitter* c = StartCCode(temp);
    
    if(iter.MergeSize() > 1 && muxInfo.size > 0){
      c->FunctionBlock(S8("MergeType"),S8("MergeTypeFromConfig"));
      c->Argument(S8("AcceleratorConfig*"),S8("config"));

      for(int i = 0; i <  muxInfo.size; i++){
        Array<MuxInfo> info = muxInfo[i];

        c->StartExpression();
        for(int j = 0; j <  info.size; j++){
          MuxInfo mux  =  info[j];

          if(j != 0){
            c->And();
          }
        
          c->Var(PushString(temp,"config->%.*s_sel",UN(mux.fullName)));
          c->IsEqual();
          c->Literal(mux.val);
        }

        c->IfOrElseIfFromExpression();

        String content = PushString(temp,"(MergeType) %d",i);
        c->Return(content);
      }

      c->Else();
      c->Statement(S8("Assert(false && \"Error recovering MergeType from current configuration\")"));
      c->EndIf();
      c->EndBlock();
    }

    Array<Array<InstanceInfo*>> unitInfoPerMerge = VUnitInfoPerMerge(info,temp);
    
    bool containsMerge = (iter.MergeSize() > 1 && muxInfo.size > 0);

    c->FunctionBlock(S8("Array<VUnitInfo>"),S8("ExtractVArguments"));
    c->Argument(S8("AcceleratorConfig*"),S8("config"));
    c->Argument(S8("int"),S8("mergeIndex"));
    c->VarDeclare(S8("int"),S8("index"),S8("0"));

    int maxMergeInfo = 0;
    for(Array<InstanceInfo*> units : unitInfoPerMerge){
      maxMergeInfo = MAX(maxMergeInfo,units.size);
    }

    c->VarDeclare(S8("static VUnitInfo"),PushString(temp,"data[%d]",maxMergeInfo));
    
    if(containsMerge){
      for(int i = 0; i <  unitInfoPerMerge.size; i++){
        Array<InstanceInfo*> units  =  unitInfoPerMerge[i];
        c->IfOrElseIf(PushString(temp,"mergeIndex == %d",i));

        for(InstanceInfo* unit : units){
          FUDeclaration* decl = unit->decl;
          String fullName = unit->fullName;
          String unitName = unit->baseName;

          c->Assignment(S8("data[index].unitName"),PushString(temp,"\"%.*s\"",UN(unitName)));
          c->Assignment(S8("data[index].mergeIndex"),PushString(temp,"%d",i));
        
          for(Wire w : decl->configs){
            String left = PushString(temp,"data[index].%.*s",UN(w.name));
            String right = PushString(temp,"config->%.*s_%.*s",UN(fullName),UN(w.name));

            c->Assignment(left,right);
          }

          c->Statement(S8("index += 1"));
        }
      }
      c->EndIf();
    } else {
      Array<InstanceInfo*> units  =  unitInfoPerMerge[0];

      for(InstanceInfo* unit : units){
        FUDeclaration* decl = unit->decl;
        String fullName = unit->fullName;
        String unitName = unit->baseName;

        c->Assignment(S8("data[0].unitName"),PushString(temp,"\"%.*s\"",UN(unitName)));
        c->Assignment(S8("data[0].mergeIndex"),S8("0"));
        
        for(Wire w : decl->configs){
          String left = PushString(temp,"data[0].%.*s",UN(w.name));
          String right = PushString(temp,"config->%.*s_%.*s",UN(fullName),UN(w.name));

          c->Assignment(left,right);
        }
      }
      c->Statement(S8("index += 1"));
    }

    c->Return(S8("(Array<VUnitInfo>){data,index}"));

    c->EndBlock();

    c->FunctionBlock(S8("void"),S8("SimulateVUnits"));
    c->Statement(S8("AcceleratorConfig* config = (AcceleratorConfig*) &configBuffer"));

    if(containsMerge){
      c->VarDeclare(S8("int"),S8("mergeIndex"),S8("MergeTypeFromConfig(config)"));
    } else {
      c->VarDeclare(S8("int"),S8("mergeIndex"),S8("0"));
    }

    for(int i = 0; i <  unitInfoPerMerge.size; i++){
      Array<InstanceInfo*> merge = unitInfoPerMerge[i];
      c->If(PushString(temp,"mergeIndex == %d",i));
      c->VarDeclare(S8("Array<VUnitInfo>"),S8("data"),S8("ExtractVArguments(config,mergeIndex)"));

      for(int k = 0; k <  merge.size; k++){
        {
          InstanceInfo* inst = merge[k];
          String sim = PushString(temp,"SIMULATE_MERGE_%d_%.*s",i,UN(inst->baseName));

          c->If(sim);

          c->VarDeclare(S8("VUnitInfo"),S8("info"),PushString(temp,"data.data[%d]",k));
          c->VarDeclare(S8("AddressVArguments"),S8("args"),S8("{}"));

          for(String str : META_AddressVParameters_Members){
            String left = PushString(temp,"args.%.*s",UN(str));
            String right = PushString(temp,"info.%.*s",UN(str));
            c->Assignment(left,right);
          }

          c->Statement(S8("PRINT(\"Simulating addresses for unit '%s' in merge config: %d\\n\",info.unitName,info.mergeIndex)"));
          c->Statement(S8("SimulateAndPrintAddressGen(args)"));

          c->EndIf();
        }
        {
          InstanceInfo* inst  =  merge[k];
          String sim = PushString(temp,"EFFICIENCY_MERGE_%d_%.*s",i,UN(inst->baseName));

          c->If(sim);

          c->VarDeclare(S8("VUnitInfo"),S8("info"),PushString(temp,"data.data[%d]",k));
          c->VarDeclare(S8("AddressVArguments"),S8("args"),S8("{}"));

          for(String str : META_AddressVParameters_Members){
            String left = PushString(temp,"args.%.*s",UN(str));
            String right = PushString(temp,"info.%.*s",UN(str));
            c->Assignment(left,right);
          }

          c->Statement(S8("SimulateVReadResult sim = SimulateVRead(args)"));
          c->Statement(S8("float percent = ((float) sim.amountOfInternalValuesUsed) / ((float) sim.amountOfExternalValuesRead)"));
          c->Statement(S8("PRINT(\"Efficiency: %2f (%d/%d)\\n\",percent,sim.amountOfInternalValuesUsed,sim.amountOfExternalValuesRead)"));
          
          c->EndIf();
        }
      }
      
      c->EndIf();
    }
    
    c->EndBlock();
    
    c->EndBlock();
    
    String content = PushASTRepr(c,temp);
    TemplateSetString("simulationStuff",content);
  }

  ProcessTemplateSimple(output,META_WrapperTemplate_Content);
}

void OutputPCEmulControl(AccelInfo info,String softwarePath){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  
  CEmitter* c = StartCCode(temp);

  Array<Array<InstanceInfo*>> unitInfoPerMerge = VUnitInfoPerMerge(info,temp);
  
  c->VarDeclare(S8("bool"),S8("debugging"),S8("false"));

  for(int i = 0; i <  unitInfoPerMerge.size; i++){
    Array<InstanceInfo*> merge  =  unitInfoPerMerge[i];
    c->Comment(PushString(temp,"Merge %d",i)); 
    for(int k = 0; k <  merge.size; k++){
      InstanceInfo* unit =  merge[k];
      String sim = PushString(temp,"SIMULATE_MERGE_%d_%.*s",i,UN(unit->baseName));
      c->VarDeclare(S8("bool"),sim,S8("false"));
      String eff = PushString(temp,"EFFICIENCY_MERGE_%d_%.*s",i,UN(unit->baseName));
      c->VarDeclare(S8("bool"),eff,S8("false"));
    }
  }
  
  FILE* file = OpenFileAndCreateDirectories(PushString(temp,"%.*s/pcEmulDefs.h",UN(softwarePath)),"w",FilePurpose_SOFTWARE);
  DEFER_CLOSE_FILE(file);
  
  String content = PushASTRepr(c,temp,true);
  fprintf(file,"%.*s",UN(content));
}

void OutputTopLevelFiles(Accelerator* accel,FUDeclaration* topLevelDecl,String hardwarePath,String softwarePath,bool isSimple,AccelInfo info,VersatComputedValues val,Array<ExternalMemoryInterface> external){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Pool<FUInstance> instances = accel->allocated;
  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(&info,temp);
  int index = 0;
  Array<Wire> allStaticsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  for(Pair<StaticId,StaticData*> p : staticUnits){
    for(Wire& config : p.second->configs){
      allStaticsVerilatorSide[index] = config;
      allStaticsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,temp);
      index += 1;
    }
  }
  allStaticsVerilatorSide.size = index;
  Array<WireInformation> wireInfo = CalculateWireInformation(instances,staticUnits,val.versatConfigs,temp);

  // Verilog includes for parameters and such
  {
    // No need for templating, small file
    FILE* c = OpenFileAndCreateDirectories(PushString(temp,"%.*s/versat_defs.vh",UN(hardwarePath)),"w",FilePurpose_VERILOG_INCLUDE);
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%.*s/versat_undefs.vh",UN(hardwarePath)),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(c);
    DEFER_CLOSE_FILE(f);
  
    if(!c || !f){
      printf("Error creating file, check if filepath is correct: %.*s\n",UN(hardwarePath));
      return;
    }

    fprintf(c,"`define NUMBER_UNITS %d\n",accel->allocated.Size());
    fprintf(f,"`undef  NUMBER_UNITS\n");
    fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
    fprintf(f,"`undef  CONFIG_W\n");
    fprintf(c,"`define STATE_W %d\n",val.stateBits);
    fprintf(f,"`undef  STATE_W\n");
    fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
    fprintf(f,"`undef  MAPPED_UNITS\n");
    fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
    fprintf(f,"`undef  MAPPED_BIT\n");
    fprintf(c,"`define nIO %d\n",val.nUnitsIO);
    fprintf(f,"`undef  nIO\n");
    fprintf(c,"`define LEN_W %d\n",20);
    fprintf(f,"`undef  LEN_W\n");

    if(globalOptions.architectureHasDatabus){
      fprintf(c,"`define VERSAT_ARCH_HAS_IO 1\n");
      fprintf(f,"`undef  VERSAT_ARCH_HAS_IO\n");
    }

    if(info.inputs || info.outputs){
      fprintf(c,"`define EXTERNAL_PORTS\n");
      fprintf(f,"`undef  EXTERNAL_PORTS\n");
    }

    if(val.nUnitsIO){
      fprintf(c,"`define VERSAT_IO\n");
      fprintf(f,"`undef  VERSAT_IO\n");
    }

    if(val.externalMemoryInterfaces){
      fprintf(c,"`define VERSAT_EXTERNAL_MEMORY\n");
      fprintf(f,"`undef  VERSAT_EXTERNAL_MEMORY\n");
    }

    if(globalOptions.exportInternalMemories){
      fprintf(c,"`define VERSAT_EXPORT_EXTERNAL_MEMORY\n");
      fprintf(f,"`undef  VERSAT_EXPORT_EXTERNAL_MEMORY\n");
    }
  }
  
  // All memory external instantiation and port mapping
  {
    String path = PushString(temp,"%.*s/versat_external_memory_inst.vh",UN(hardwarePath));
    FILE* f = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_INCLUDE);
    if(!f){
      // TODO
    }
    String content = EmitExternalMemoryInstances(external,temp);
    fprintf(f,"%.*s",UNPACK_SS(content));
    fclose(f);
  }

  {
    String path = PushString(temp,"%.*s/versat_internal_memory_wires.vh",UN(hardwarePath));
    FILE* f = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_INCLUDE);
    if(!f){
      // TODO
    }
    String content = EmitInternalMemoryWires(external,temp);
    fprintf(f,"%.*s",UNPACK_SS(content));
    fclose(f);
  }

  {
    String path = PushString(temp,"%.*s/versat_external_memory_port.vh",UN(hardwarePath));
    FILE* f = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_INCLUDE);
    if(!f){
      // TODO
    }
    String content = EmitExternalMemoryPort(external,temp);
    fprintf(f,"%.*s",UNPACK_SS(content));
    fclose(f);
  }

  {
    String path = PushString(temp,"%.*s/versat_external_memory_internal_portmap.vh",UN(hardwarePath));
    FILE* f = OpenFileAndCreateDirectories(path,"w",FilePurpose_VERILOG_INCLUDE);
    if(!f){
      // TODO
    }
    String content = EmitExternalMemoryInternalPortmap(external,temp);
    fprintf(f,"%.*s",UNPACK_SS(content));
    fclose(f);
 }

  // Top configurations
  // NOTE: Versat instance does not instantiate configurations if not config bits and therefore we do not emit it (simplifies logic has otherwise we would have to worry about emitting something passable. This way we can just skip stuff).
  if(val.configurationBits){
    FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%.*s/versat_configurations.v",UN(hardwarePath)),"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(s);

    if(!s){
      printf("Error creating file, check if filepath is correct: %.*s\n",UN(hardwarePath));
      return;
    }

    String content = EmitConfiguration(val,wireInfo,temp);
    fprintf(s,"%.*s\n",UNPACK_SS(content));
  }

  Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.infos[0].info,temp);

  OutputTopLevel(accel,allStaticsVerilatorSide,info,topLevelDecl,structuredConfigs,hardwarePath,val,external,wireInfo);
  OutputHeader(structuredConfigs,info,isSimple,accel,softwarePath,allStaticsVerilatorSide,val);
  
  OutputVerilatorWrapper(accel,allStaticsVerilatorSide,info,topLevelDecl,structuredConfigs,softwarePath);
  OutputMakefile(accel->name,topLevelDecl,softwarePath);
  OutputPCEmulControl(info,softwarePath);

  {
    // TODO: Need to add some form of error checking and handling inside the script for the case where verilator root is not found
    String versatPath = PushString(temp,"%.*s/iob_versat.v",UNPACK_SS(globalOptions.hardwareOutputFilepath));
    FILE* output = OpenFileAndCreateDirectories(versatPath,"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(output);

    ClearTemplateEngine();

    TemplateSetString("prefix",globalOptions.prefixIObPort);
    
    if(globalOptions.extraIOb){
      TemplateSetString("extraIob",S8("iob_csrs_iob_rready_i,"));
    } else {
      TemplateSetString("extraIob",{});
    }

    if(globalOptions.useSymbolAddress){
      TemplateSetString("AXIAddr",R"FOO(
assign axi_awaddr_o = temp_axi_awaddr_o[AXI_ADDR_W-3:2];
assign axi_araddr_o = temp_axi_araddr_o[AXI_ADDR_W-3:2];
)FOO");
    } else {
      TemplateSetString("AXIAddr",R"FOO(
assign axi_awaddr_o = temp_axi_awaddr_o;
assign axi_araddr_o = temp_axi_araddr_o;
)FOO");
    }

    if(globalOptions.useSymbolAddress){
      TemplateSetString("addr",PushString(temp,".csr_addr({%.*siob_addr_i,2'b00}),",UN(globalOptions.prefixIObPort)));
    } else {
      TemplateSetString("addr",PushString(temp,".csr_addr(%.*siob_addr_i),",UN(globalOptions.prefixIObPort)));
    }
    
    ProcessTemplateSimple(output,META_VersatTemplate_Content);
  }
  
  {
    // TODO: Need to add some form of error checking and handling inside the script for the case where verilator root is not found
    String getVerilatorScriptPath = PushString(temp,"%.*s/GetVerilatorRoot.sh",UNPACK_SS(globalOptions.softwareOutputFilepath));
    FILE* output = OpenFileAndCreateDirectories(getVerilatorScriptPath,"w",FilePurpose_SCRIPT);
    DEFER_CLOSE_FILE(output);

    fprintf(output,"%.*s",UN(META_GetVerilatorRoot_Content));
    fflush(output);
    OS_SetScriptPermissions(output);
  }

  {
    String extractVerilatedSignalPath = PushString(temp,"%.*s/ExtractVerilatedSignals.py",UNPACK_SS(globalOptions.softwareOutputFilepath));
    FILE* output = OpenFileAndCreateDirectories(extractVerilatedSignalPath,"w",FilePurpose_SCRIPT);
    DEFER_CLOSE_FILE(output);

    fprintf(output,"%.*s",UNPACK_SS(META_ExtractVerilatedSignals_Content));
    fflush(output);
    OS_SetScriptPermissions(output);
  }
}
