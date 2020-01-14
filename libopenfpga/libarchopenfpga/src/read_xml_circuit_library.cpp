/********************************************************************
 * This file includes the top-level function of this library
 * which reads an XML modeling OpenFPGA architecture to the associated
 * data structures
 *******************************************************************/
#include <string>

/* Headers from pugi XML library */
#include "pugixml.hpp"
#include "pugixml_util.hpp"

/* Headers from libarchfpga */
#include "arch_error.h"
#include "read_xml_util.h"

#include "read_xml_circuit_library.h"

/********************************************************************
 * Convert string to the enumerate of model type
 *******************************************************************/
static 
e_circuit_model_type string_to_circuit_model_type(const std::string& type_string) {
  if (std::string("chan_wire") == type_string) {
    return CIRCUIT_MODEL_CHAN_WIRE;
  }

  if (std::string("wire") == type_string) {
    return CIRCUIT_MODEL_WIRE;
  }

  if (std::string("mux") == type_string) {
    return CIRCUIT_MODEL_MUX;
  }

  if (std::string("lut") == type_string) {
    return CIRCUIT_MODEL_LUT;
  }

  if (std::string("ff") == type_string) {
    return CIRCUIT_MODEL_FF;
  }

  if (std::string("sram") == type_string) {
    return CIRCUIT_MODEL_SRAM;
  }

  if (std::string("hard_logic") == type_string) {
    return CIRCUIT_MODEL_HARDLOGIC;
  }

  if (std::string("ccff") == type_string) {
    return CIRCUIT_MODEL_CCFF;
  }

  if (std::string("iopad") == type_string) {
    return CIRCUIT_MODEL_IOPAD;
  }

  if (std::string("inv_buf") == type_string) {
    return CIRCUIT_MODEL_INVBUF;
  }

  if (std::string("pass_gate") == type_string) {
    return CIRCUIT_MODEL_PASSGATE;
  }

  if (std::string("gate") == type_string) {
    return CIRCUIT_MODEL_GATE;
  }

  /* Reach here, we have an invalid value, error out */
  return NUM_CIRCUIT_MODEL_TYPES;
}

/********************************************************************
 * Convert string to the enumerate of model type
 *******************************************************************/
static 
e_circuit_model_design_tech string_to_design_tech_type(const std::string& type_string) {
  if (std::string("cmos") == type_string) {
    return CIRCUIT_MODEL_DESIGN_CMOS;
  }

  if (std::string("rram") == type_string) {
    return CIRCUIT_MODEL_DESIGN_RRAM;
  }

  return NUM_CIRCUIT_MODEL_DESIGN_TECH_TYPES;
}

/********************************************************************
 * Parse XML codes of design technology of a circuit model to circuit library
 *******************************************************************/
static 
void read_xml_model_design_technology(pugi::xml_node& xml_model,
                                      const pugiutil::loc_data& loc_data,
                                      CircuitLibrary& circuit_lib, const CircuitModelId& model) {

  auto xml_design_tech = get_single_child(xml_model, "design_technology", loc_data); 

  /* Identify if the circuit model power-gated */
  circuit_lib.set_model_is_power_gated(model, get_attribute(xml_design_tech, "power_gated", loc_data, pugiutil::ReqOpt::OPTIONAL).as_bool(false));

  /* Identify the type of design technology */
  const char* type_attr = get_attribute(xml_design_tech, "type", loc_data).value();
  /* Translate the type of design technology to enumerate */
  e_circuit_model_design_tech design_tech_type = string_to_design_tech_type(std::string(type_attr));

  if (NUM_CIRCUIT_MODEL_DESIGN_TECH_TYPES == design_tech_type) {
    archfpga_throw(loc_data.filename_c_str(), loc_data.line(xml_design_tech),
                   "Invalid 'type' attribute '%s'\n",
                   type_attr);
  }

  circuit_lib.set_model_design_tech_type(model, design_tech_type);
   
  /* Parse exclusive attributes for inverters and buffers */
  
}

/********************************************************************
 * Parse XML codes of a circuit model to circuit library
 *******************************************************************/
static 
void read_xml_circuit_model(pugi::xml_node& xml_model,
                            const pugiutil::loc_data& loc_data,
                            CircuitLibrary& circuit_lib) {
  /* Find the type of the circuit model
   * so that we can add a new circuit model to circuit library 
   */
  const char* type_attr = get_attribute(xml_model, "type", loc_data).value();

  /* Translate the type of circuit model to enumerate */
  e_circuit_model_type model_type = string_to_circuit_model_type(std::string(type_attr));

  if (NUM_CIRCUIT_MODEL_TYPES == model_type) {
    archfpga_throw(loc_data.filename_c_str(), loc_data.line(xml_model),
                   "Invalid 'type' attribute '%s'\n",
                   type_attr);
  }

  CircuitModelId model = circuit_lib.add_model(model_type);

  /* Find the name of the circuit model */
  const char* name_attr = get_attribute(xml_model, "name", loc_data).value();
  circuit_lib.set_model_name(model, std::string(name_attr));

  /* TODO: This attribute is going to be DEPRECATED 
   * Find the prefix of the circuit model
   */
  const char* prefix_attr = get_attribute(xml_model, "prefix", loc_data).value();
  circuit_lib.set_model_prefix(model, std::string(prefix_attr));

  /* Find a SPICE netlist which is an optional attribute*/
  circuit_lib.set_model_circuit_netlist(model, get_attribute(xml_model, "spice_netlist", loc_data, pugiutil::ReqOpt::OPTIONAL).as_string(""));

  /* Find a Verilog netlist which is an optional attribute*/
  circuit_lib.set_model_verilog_netlist(model, get_attribute(xml_model, "verilog_netlist", loc_data, pugiutil::ReqOpt::OPTIONAL).as_string(""));

  /* Find if the circuit model is default in its type */
  circuit_lib.set_model_is_default(model, get_attribute(xml_model, "is_default", loc_data, pugiutil::ReqOpt::OPTIONAL).as_bool(false));
  
  /* Find if the circuit model is should be dumped in structural verilog */
  circuit_lib.set_model_dump_structural_verilog(model, get_attribute(xml_model, "dump_structural_verilog", loc_data, pugiutil::ReqOpt::OPTIONAL).as_bool(false));

  /* Parse attributes under the <circuit_model> */
  /* Design technology -related attributes */
  read_xml_model_design_technology(xml_model, loc_data, circuit_lib, model);
  
}

/********************************************************************
 * Parse XML codes about circuit models to circuit library
 *******************************************************************/
CircuitLibrary read_xml_circuit_library(pugi::xml_node& Node,
                                        const pugiutil::loc_data& loc_data) {
  CircuitLibrary circuit_lib;

  /* Iterate over the children under this node,
   * each child should be named after circuit_model
   */
  for (pugi::xml_node xml_model : Node.children()) {
    /* Error out if the XML child has an invalid name! */
    if (xml_model.name() != std::string("circuit_model")) {
      bad_tag(xml_model, loc_data, Node, {"circuit_model"});
    }
    read_xml_circuit_model(xml_model, loc_data, circuit_lib);
  } 

  return circuit_lib;
}
