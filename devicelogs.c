#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>


/*
#USAGE
  ./devicelogs <input_file> <output_file> <conversion_type> -seperator <1|2|3> -opsys <1|2|3> [-h]
#ARGUMENTS
  <input_file> : Source file to read the data from.

  <output_file> : The output file to write the converted data.

  <conversion_type> : The type of conversion to perform 
  1 : CSV to binary 
  2 : Binary to XML
  3 : XSD Validation

  -seperator <1|2|3> : The separator used in the CSV file (1 for comma, 2 for space, 3 for semicolon).

  -opsys <1|2|3> : The operating system type (1 for Windows, 2 for Linux, 3 for Mac).

#OPTIONS
  -h : Display this help message.

#EXAMPLES
    ./devicelogs smartlogs.csv logdata.dat 1 -seperator 1 -opsys 1
    ./devicelogs logdata.dat logdata.xml 2 -seperator 2 -opsys 2
    ./devicelogs logdata.xml logdata.xsd 3 -seperator 3 -opsys 3
    ./devicelogs -h

*/
typedef struct {
    char device_id[10];
    char timestamp[20];
    float temperature;
    float humidity;
    char status[10];
    char location[31];
    char alert_level[10];
    int battery;
    char firmware_ver[20];
    uint8_t event_code;
} DeviceLog;

// Function to convert CSV to binary
void csv_to_binary(int seperator_choice, int opsys_choice, FILE *input_file, FILE *output_file) {
    FILE *csvFile = fopen(input_file,"r");    // r is basically read mode.
    FILE *datFile = fopen(output_file,"wb"); //With wb, we can open a file for writing in binary system.

    if(csvFile == NULL || datFile == NULL){
        printf("File not found.");
    }
    else {
        char *seperator = NULL;
        char end_of_line[4]="\r\n";
        char line[200];
        switch (seperator_choice)
        {
        case 1:
            seperator = ",";
            break;
        case 2:
            seperator = " ";
            break;
        case 3:
            seperator = ";";
            break;
        default:
            seperator = ",";
            break;
        }
        switch (opsys_choice)
{
    case 1:
        strcpy(end_of_line, "\r\n");
        break;
    case 2:
        strcpy(end_of_line, "\r");
        break;
    case 3:
        strcpy(end_of_line, "\n");
        break;
    default:
        strcpy(end_of_line, "\r\n");
        break;
}
        
        fgets(line,sizeof(line),csvFile);//skip the first line
        while (fgets(line,sizeof(line),csvFile))
        {
            char *ptrLine = line;

            DeviceLog device;
            char *token;

            char *crlf = strpbrk(line, end_of_line);
            if(crlf != NULL) {
                *crlf = '\0';//satır sonu karakteri \0 ile değiştirilir.
            }


            //device id
            token = strsep(&ptrLine,seperator);//strtok yerine strsep kullanılmasının nedeni strtok'un boşlukları atlaması.
            if(*token != '\0'){
                strcpy(device.device_id, token);
            } else{
                strcpy(device.device_id, "N/A");
            }

            //timestampt
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                strcpy(device.timestamp,token);
            } else {
                strcpy(device.timestamp, "N/A");
            }

            //temperature
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                device.temperature = atof(token);
            } else{
                device.temperature = -999.0;
            }

            //humidity
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                device.humidity = atof(token);
            } else{
                device.humidity = -1.0;
            }
            
            //status
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                strcpy(device.status,token);
            } else{
                strcpy(device.status, "N/A");
            }

            //location
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                strcpy(device.location,token);
            } else{
                strcpy(device.location, "N/A");
            }

            //alert level
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                strcpy(device.alert_level,token);
            } else{
                strcpy(device.alert_level, "N/A");
            }

            //battery
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                device.battery = atoi(token);
            } else{
                device.battery = -1;
            }

            //firmware version
            token = strsep(&ptrLine,seperator);
            if(*token != '\0')
            {
                strcpy(device.firmware_ver,token);
            } else{
                strcpy(device.firmware_ver, "N/A");
            }

            //event code
            token = strsep(&ptrLine,seperator);
            if (*token != '\0' && atoi(token) >= 0 && atoi(token) <= 255)
            {
                device.event_code = (uint8_t)atoi(token);
            } else{
                device.event_code = (uint8_t)0;
            }

            fwrite(&device, sizeof(device) , 1, datFile);
        }
        

    }

    fclose(csvFile);
    fclose(datFile);
    printf("CSV to binary conversion completed successfully.\n");
}

// Functions to convert binary data to XML
void binary_to_xml(){
    int keyStart, keyEnd;
    char order[4], dataFileName[100];

    if (read_setup_params("setupParams.json", &keyStart, &keyEnd, order, dataFileName)) {
        return 1;
    }

    printf("Key Start: %d\n", keyStart);
    printf("Key End: %d\n", keyEnd);
    printf("Order: %s\n", order);
    printf("Data File Name: %s\n", dataFileName);

    int recordCount;
    Record *records;
    if (read_binary_file(dataFileName, keyStart, keyEnd, &recordCount, &records)) {
        return 1;
    }
    //qsort(records, recordCount, sizeof(Record), compare_records);

    generate_xml("smartlogs.xml", records, recordCount);

    /*for (int i = 0; i < recordCount; i++) {
        free(records[i].data);
    }*/
    free(records);

    printf("XML file generated successfully.\n");
}
int compare_records(const void *a, const void *b) {
    Record *recordA = (Record *)a;
    Record *recordB = (Record *)b;
    
    if (strcmp(recordA-> location, "ASC") == 0) {
        return recordA->device_id - recordB->device_id;
    } else {
        return recordB->device_id - recordA->device_id;
    }
}
int read_setup_params(const char *filename, int *keyStart, int *keyEnd, char *order, char *dataFileName) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening JSON configuration file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_data = malloc(file_size + 1);
    fread(json_data, 1, file_size, file);
    fclose(file);

    struct json_object *jobj = json_tokener_parse(json_data);
    free(json_data);
    if (jobj == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return 1;
    }

    struct json_object *jKeyStart = json_object_object_get(jobj, "keyStart");
    struct json_object *jKeyEnd = json_object_object_get(jobj, "keyEnd");
    struct json_object *jOrder = json_object_object_get(jobj, "order");
    struct json_object *jDataFileName = json_object_object_get(jobj, "dataFileName");

    *keyStart = json_object_get_int(jKeyStart);
    *keyEnd = json_object_get_int(jKeyEnd);
    strcpy(order, json_object_get_string(jOrder));
    strcpy(dataFileName, json_object_get_string(jDataFileName));

    json_object_put(jobj);

    return 0;
}
int read_binary_file(const char *filename, int keyStart, int keyEnd, int *recordCount, Record **records) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error opening binary file\n");
        return 1;
    }

    fseek(file, keyStart, SEEK_END);
    long file_size = ftell(file);
    fseek(file, keyStart, SEEK_SET);

    int recordSize = keyEnd - keyStart + 1;
    *recordCount = file_size / sizeof(Record);
    *records = malloc(sizeof(Record) * (*recordCount));

    for (int i = 0; i < *recordCount; i++) { 
        fread(&(*records)[i], sizeof(Record), 1, file);
    }

    fclose(file);
    return 0;
}
void generate_xml(const char *filename, Record *records, int recordCount) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_element = xmlNewNode(NULL, BAD_CAST "smartlogs");
    xmlDocSetRootElement(doc, root_element);

    for (int i = 0; i < recordCount; i++) {
        xmlNodePtr entry = xmlNewChild(root_element, NULL, BAD_CAST "entry", NULL);
        char tempID[10];
        snprintf(tempID, sizeof(tempID), "%d", i+1);//converting int to string
        xmlNewProp(entry, BAD_CAST "id", BAD_CAST tempID);//adding attribute to entry node(id is starting from 1)

        xmlNodePtr device = xmlNewChild(entry, NULL, BAD_CAST "device", BAD_CAST "");

        xmlNewChild(device, NULL, BAD_CAST "device_id", BAD_CAST records[i].device_id);
        xmlNewChild(device, NULL, BAD_CAST "location", BAD_CAST records[i].location);
        xmlNewChild(device, NULL, BAD_CAST "firmware_ver", BAD_CAST records[i].firmware_ver);

        xmlNodePtr metrics = xmlNewChild(entry, NULL, BAD_CAST "metrics", NULL);
        xmlNewProp(metrics, BAD_CAST "status", BAD_CAST records[i].status);
        xmlNewProp(metrics, BAD_CAST "alert_level", BAD_CAST records[i].alert_level);

        char strTmprtr[10];//we dont have a function to write float value to xml
        snprintf(strTmprtr, sizeof(strTmprtr), "%f", records[i].temperature);//converting float to string
        xmlNewChild(metrics, NULL, BAD_CAST "temperature", BAD_CAST strTmprtr);

        char strHumidity[10];
        snprintf(strHumidity, sizeof(strHumidity), "%f", records[i].humidity);
        xmlNewChild(metrics, NULL, BAD_CAST "humidity", BAD_CAST strHumidity);

        char strBattery[10];
        snprintf(strBattery, sizeof(strBattery), "%d", records[i].battery);
        xmlNewChild(metrics, NULL, BAD_CAST "battery", BAD_CAST strBattery);

        xmlNewChild(entry, NULL, BAD_CAST "timestamp", BAD_CAST records[i].timestamp);

        char strEventCode[10];
        snprintf(strEventCode, sizeof(strEventCode), "%d", records[i].event_code);
        xmlNodePtr event_code = xmlNewChild(entry, NULL, BAD_CAST "event_code", BAD_CAST strEventCode);
        xmlNewProp(event_code, BAD_CAST "", BAD_CAST "");//Attribute olarak big endian değerleri eklenecek
    }

    xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

    xmlFreeDoc(doc);
}

// Function to validate XML against XSD
void validate_xml(const char *xmlFileName, const char *xsdFileName) {
    xmlDocPtr doc = xmlParseFile(xmlFileName);
    if (doc == NULL) {
        fprintf(stderr, "Error: could not parse file %s\n", xmlFileName);
        return;
    }

    xmlSchemaParserCtxtPtr parserCtxt = xmlSchemaNewParserCtxt(xsdFileName);
    xmlSchemaPtr schema = xmlSchemaParse(parserCtxt);
    xmlSchemaValidCtxtPtr validCtxt = xmlSchemaNewValidCtxt(schema);

    if (xmlSchemaValidateDoc(validCtxt, doc) == 0) {
        printf("XML file is valid.\n");
    } else {
        printf("XML file is invalid.\n");
    }

    xmlSchemaFreeValidCtxt(validCtxt);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(parserCtxt);
    xmlFreeDoc(doc);
}


// Main function

int main(FILE input_file, FILE output_file, int conversion_type, int seperator_choice, int opsys_choice)
{
    if (strcmp(input_file, "-h") == 0) {
        printf("Usage: ./devicelogs <input_file> <output_file> <conversion_type> -seperator <1|2|3> -opsys <1|2|3> [-h]\n");
        return 0;
    }

    if (strcmp(conversion_type, "1") == 0) {
        csv_to_binary(seperator_choice, opsys_choice, input_file, output_file);
    } else if (strcmp(conversion_type, "2") == 0) {
        binary_to_xml();
    } else if (strcmp(conversion_type, "3") == 0) {
        validate_xml(input_file, output_file);
    } else {
        printf("Invalid conversion type.\n");
        return 1;
    }

    return 0;
}

