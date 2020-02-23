#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "./src/ValueMapping/Valuemapping.h"
#include "webserver_map_fnc.h"


extern VALUEMAPPING SensorMapping;

static  WebServer* MappingServer = nullptr;
static char supportedsensors_chr[4096]; //4k Memory for the JSON file
static char connectedsensors_chr[4096]; //4k Memory for the JSON file

void generate_static_supportedsensors_json( void );
void generate_static_connectedsensors_json( void );

void response_mappingdata( void );
void response_supportedsensors( void );
void response_connectedsensors( void );
void process_setmapping( void );
void response_channelvalue( void );

/**************************************************************************************************
 *    Function      : Webserver_Map_FunctionsRegister
 *    Description   : Registers new URL for handling
 *    Input         : WebServer* serverptr 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void Webserver_Map_FunctionsRegister( WebServer* serverptr){
MappingServer=serverptr;
MappingServer->on("/mapping/mappingdata.json", HTTP_GET, response_mappingdata);
MappingServer->on("/devices/supportedsensors.json",HTTP_GET,response_supportedsensors);
MappingServer->on("/devices/connectedsensors.json",HTTP_GET,response_connectedsensors);
MappingServer->on("/mapping/set",HTTP_POST,process_setmapping ); //Sets a single mapping for one channel
MappingServer->on("/mapping/{}/value",HTTP_GET, response_channelvalue); //Not recommended for polling!

//We need here to generate the static data for 
// - supportedsensors.json 
// - connectedsensors.json
generate_static_supportedsensors_json();
generate_static_connectedsensors_json();

}


void generate_static_supportedsensors_json( void ){
    //Data will be created at boottime, so we only need to deliver the generated data here
    VALUEMAPPING::SensorElementEntry_t SensorList[64];
    uint8_t SensorCount = SensorMapping.GetSensors(SensorList,64);
    //Serial.printf("Sensors supported: %i \n\r", SensorCount);
    /*
          SensorBus_t Bus;
            DATAUNITS::MessurmentValueType_t ValueType;
            uint8_t ChannelIDX;
    */
    const size_t capacity = JSON_ARRAY_SIZE(64) + JSON_OBJECT_SIZE(1) + 64*JSON_OBJECT_SIZE(3);
    DynamicJsonDocument doc(capacity);

    
    JsonArray Mapping = doc.createNestedArray("SensorList");
    for(uint32_t i=0;i<( SensorCount  );i++){
            JsonObject MappingObj = Mapping.createNestedObject();
            MappingObj["Bus"] = (uint8_t)(SensorList[i].Bus);
            MappingObj["ValueType"] = (uint8_t)(SensorList[i].ValueType);
            MappingObj["Channel"] = (uint8_t)(SensorList[i].ChannelIDX); 
            MappingObj["Name"]= SensorMapping.GetSensorName( SensorList[i]);
    }
    serializeJson(doc, supportedsensors_chr, sizeof(supportedsensors_chr) ); //We use up static allocated memory
}


void generate_static_connectedsensors_json( void ){
    //We need to collect a whole buch of data for this one......
    VALUEMAPPING::SensorElementEntry_t SensorList[64];
    uint8_t SensorCount = SensorMapping.GetConnectedSensors(SensorList,64);
    //Serial.printf("Sensors supported: %i \n\r", SensorCount);
    String data="";
    /*
          SensorBus_t Bus;
            DATAUNITS::MessurmentValueType_t ValueType;
            uint8_t ChannelIDX;
    */
    const size_t capacity = JSON_ARRAY_SIZE(64) + JSON_OBJECT_SIZE(1) + 64*JSON_OBJECT_SIZE(3);
    DynamicJsonDocument doc(capacity);

    
    JsonArray Mapping = doc.createNestedArray("SensorList");
    for(uint32_t i=0;i<( SensorCount  );i++){
            JsonObject MappingObj = Mapping.createNestedObject();
            MappingObj["Bus"] = (uint8_t)(SensorList[i].Bus);
            MappingObj["ValueType"] = (uint8_t)(SensorList[i].ValueType);
            MappingObj["Channel"] = (uint8_t)(SensorList[i].ChannelIDX);
            MappingObj["Name"]= SensorMapping.GetSensorName( SensorList[i]);
    }
    serializeJson(doc, connectedsensors_chr, sizeof(connectedsensors_chr) ); //We use up static allocated memory
}


/**************************************************************************************************
 *    Function      : process_setmapping
 *    Description   : Processes POST for new mapping
 *    Input         : none 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void process_setmapping( void ){
//We need here the element eqivalent for the mapping
//Also we make sure that this is a valid element
bool completedata = true;

int32_t iBus=0;
int32_t iMappedChannel=0;
int32_t iValueType=0;
int32_t iValueChannel=0;


  if( ! MappingServer->hasArg("MAPPEDCHANNEL") || MappingServer->arg("MAPPEDCHANNEL") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
    completedata = completedata & false;
  } else {
     iMappedChannel = MappingServer->arg("MAPPEDCHANNEL").toInt();
  }

  if( ! MappingServer->hasArg("BUS") || MappingServer->arg("BUS") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
    completedata = completedata & false;
  } else {
     iBus= MappingServer->arg("BUS").toInt();
    
  }

  if( ! MappingServer->hasArg("MESSURMENTVALTYPE") || MappingServer->arg("MESSURMENTVALTYPE") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
    completedata = completedata & false;
  } else {
     iValueType = MappingServer->arg("MESSURMENTVALTYPE").toInt();
  }

  if( ! MappingServer->hasArg("VALCHANNEL") || MappingServer->arg("VALCHANNEL") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
    completedata = completedata & false;
  } else {
    iValueChannel = MappingServer->arg("VALCHANNEL").toInt();
  }

  if(true == completedata){
      //SaintyCheck
      bool fault = false;
      if( (iBus<0) || ( iBus>=VALUEMAPPING::SensorBus_t::SENSORBUS_CNT) ){
          //Out of range
          Serial.println("iBus out of range");
          fault=true;
      }

      if( ( iValueType<0 ) || (iValueType >= DATAUNITS::MessurmentValueType_t::MESSUREMENTVALUE_CNT) ){
          //Out of range
          Serial.println("iValueType out of range");
          fault=true;
      }

      if( (iValueChannel<0) || (iValueChannel>255) ){
          //Out of range
          Serial.println("iValueChannel out of range");
          fault=true;
      }

      if( ( iMappedChannel <0) || (iMappedChannel>=64) ){
          Serial.println("iMappedChannel out of range");
          fault=true;
      }

      if(false == fault ){
        //We can try to buld the element
        
        Serial.println("Data Received");
        Serial.printf("iBus=%i ,",iBus);
        Serial.printf("iValueType=%i ,",iValueType);
        Serial.printf("iValueChannel=%i ,",iValueChannel);
        Serial.printf("iMappedChannel=%i \n\r",iMappedChannel);
        
        VALUEMAPPING::SensorElementEntry_t Element;
        Element.Bus = (VALUEMAPPING::SensorBus_t)(iBus);
        Element.ValueType = (DATAUNITS::MessurmentValueType_t )(iValueType);
        Element.ChannelIDX = (uint8_t)iValueChannel;
        SensorMapping.SetMappingForChannel(iMappedChannel, Element);
        Serial.println("Upadte Mapping");
      } else {

      }

  } else {

  }

  MappingServer->send(200); 

}

/**************************************************************************************************
 *    Function      : response_mappingdata
 *    Description   : Sends mappingdatat to client
 *    Input         : none 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void response_mappingdata( void ){
    if(SPIFFS.exists("/mapping.json")){
      File file = SPIFFS.open("/mapping.json", "r");
      MappingServer->streamFile(file, "application/json");
      file.close();
    } else {
      String data="{}";
      MappingServer->send(200, "application/json", data);
    }
}

/**************************************************************************************************
 *    Function      : response_connectedsensors
 *    Description   : Sends connected sensors to client
 *    Input         : none 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void response_connectedsensors( void ){
    MappingServer->send_P(200, "application/json", connectedsensors_chr);
}

/**************************************************************************************************
 *    Function      : response_supportedsensors
 *    Description   : Sends supported sensors to client
 *    Input         : none 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void response_supportedsensors( void ){
    MappingServer->send_P(200, "application/json", supportedsensors_chr);
}

/**************************************************************************************************
 *    Function      : response_channelvalue
 *    Description   : Sends the value of a channel back to the client
 *    Input         : none 
 *    Output        : none
 *    Remarks       : None
 **************************************************************************************************/
void response_channelvalue( void ){
    float value = 0;
    StaticJsonDocument<512> root; //This is allocated in stack
    String response = ""; //Will fragment heap
    WebServer * server = MappingServer;
    if(server == nullptr){
        return;
    }
   
    uint8_t max_channel = SensorMapping.GetMaxMappedChannels();
    String IDs = server->pathArg(0);//This will be the channel we like to set
    int32_t Ch = IDs.toInt();
    if( (Ch<0) ){
        Ch=0;
    }

    if(Ch>=max_channel){
        Ch=max_channel-1; 
    }
    //Reading is in general a bad idea here as this will take a long time...
    bool mapped = SensorMapping.ReadMappedValue( &value, Ch );

    //We build a qick JSON 
    root["channel"]= Ch;
    root["mapped"]= (bool)mapped;
    root["value"] = value ;
    serializeJson(root, response);
    server->send(200, "application/json", response);
  
}

