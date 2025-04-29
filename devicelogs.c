#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemastypes.h>

typedef struct {
    char device_id[10];
    char timestamp[20];
    float temperature;
    int humidity;
    char status[10];
    char location[50];
    char alert_level[10];
    int battery;
    char firmware_ver[20];
    uint8_t event_code;
} DeviceLog;

int keyStartGlobal;
int keyEndGlobal;
char orderGlobal[4];

typedef struct {
    DeviceLog record;
    unsigned char key[10];
} RecordWithKey;

// Corrected csv_to_binary function
void csv_to_binary(int separator_choice, int opsys_choice, const char *input_filename, const char *output_filename) {
    FILE *csvFile = fopen(input_filename, "r");    
    FILE *datFile = fopen(output_filename, "wb"); 

    if (csvFile == NULL || datFile == NULL) {
        printf("File not found.\n");
        return;
    } else {
        char *separator = NULL;
        char end_of_line[4] = "\r\n";
        char line[200];

        switch (separator_choice) {
            case 1: separator = ","; break;
            case 2: separator = " "; break;
            case 3: separator = ";"; break;
            default: separator = ","; break;
        }
        switch (opsys_choice) {
            case 1: strcpy(end_of_line, "\r\n"); break;
            case 2: strcpy(end_of_line, "\r"); break;
            case 3: strcpy(end_of_line, "\n"); break;
            default: strcpy(end_of_line, "\r\n"); break;
        }

        fgets(line, sizeof(line), csvFile); // skip header line

        while (fgets(line, sizeof(line), csvFile)) {
            char *ptrLine = line;
            DeviceLog device;
            char *token;

            char *crlf = strpbrk(line, end_of_line);
            if (crlf != NULL) *crlf = '\0';

            token = strsep(&ptrLine, separator);
            strcpy(device.device_id, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            strcpy(device.timestamp, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            device.temperature = (*token != '\0') ? atof(token) : -999.0f;

            token = strsep(&ptrLine, separator);
            device.humidity = (*token != '\0') ? atoi(token) : -1;

            token = strsep(&ptrLine, separator);
            strcpy(device.status, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            strcpy(device.location, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            strcpy(device.alert_level, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            device.battery = (*token != '\0') ? atoi(token) : -1;

            token = strsep(&ptrLine, separator);
            strcpy(device.firmware_ver, (*token != '\0') ? token : "N/A");

            token = strsep(&ptrLine, separator);
            device.event_code = (*token != '\0') ? (uint8_t)atoi(token) : 0;

            fwrite(&device, sizeof(DeviceLog), 1, datFile);
        }
    }

    fclose(csvFile);
    fclose(datFile);
    printf("CSV to binary conversion completed successfully.\n");
}

// Helper functions (fixed signatures)

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

int compareKeys(const void *a,const void *b) {
    RecordWithKey *recA = (RecordWithKey*)a;
    RecordWithKey *recB = (RecordWithKey*)b;

    int cmp = memcmp(recA->key, recB->key, keyStartGlobal - keyEndGlobal + 1);

    if(strcmp(orderGlobal, "ASC") == 0){
        return cmp;
    } else if(strcmp(orderGlobal, "DESC") == 0) {
        return -cmp;
    }
} 

int read_binary_file(const char *filename, int keyStart, int keyEnd, int *recordCount, RecordWithKey **records) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error opening binary file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *recordCount = file_size / sizeof(DeviceLog);
    *records = malloc(sizeof(RecordWithKey) * (*recordCount));

    for (int i = 0; i < *recordCount; i++) {
        fread(&(*records)[i].record, sizeof(DeviceLog), 1, file);
        memcpy(&(*records)[i].key, ((char*)&(*records)[i].record) + keyStart, keyEnd - keyStart + 1);//keyend - keystart + 1 byte kadar veri structtaki key alanına kopyalanır.
    }

    qsort(*records, *recordCount, sizeof(RecordWithKey), compareKeys);

    fclose(file);
    return 0;
}

void generate_xml(const char *filename, RecordWithKey *records, int recordCount) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_element = xmlNewNode(NULL, BAD_CAST "smartlogs");
    xmlDocSetRootElement(doc, root_element);

    for (int i = 0; i < recordCount; i++) {
        DeviceLog record = records[i].record;
        xmlNodePtr entry = xmlNewChild(root_element, NULL, BAD_CAST "entry", NULL);
        char tempID[10];
        snprintf(tempID, sizeof(tempID), "%d", i + 1);
        xmlNewProp(entry, BAD_CAST "id", BAD_CAST tempID);
        
        xmlNodePtr device = xmlNewChild(entry, NULL, BAD_CAST "device", NULL);
        xmlNewChild(device, NULL, BAD_CAST "device_id", BAD_CAST record.device_id);

        if(strcmp(record.location, "N/A") != 0) {
            xmlNewChild(device, NULL, BAD_CAST "location", BAD_CAST record.location);
        }

        if(strcmp(record.firmware_ver, "N/A") != 0) {
            xmlNewChild(device, NULL, BAD_CAST "firmware_ver", BAD_CAST record.firmware_ver);
        }
        
        
        

        xmlNodePtr metrics = xmlNewChild(entry, NULL, BAD_CAST "metrics", NULL);
        if(strcmp(record.status, "N/A") != 0) {
            xmlNewProp(metrics, BAD_CAST "status", BAD_CAST record.status);
        }
        if(strcmp(record.alert_level, "N/A") != 0) {
            xmlNewProp(metrics, BAD_CAST "alert_level", BAD_CAST record.alert_level);
        }

        char temp[20];
        snprintf(temp, sizeof(temp), "%.2f", record.temperature);
        if(record.temperature != -999.0f) {
            xmlNewChild(metrics, NULL, BAD_CAST "temperature", BAD_CAST temp);
        }
        
        snprintf(temp, sizeof(temp), "%d", record.humidity);
        if(record.humidity != -1) {
            xmlNewChild(metrics, NULL, BAD_CAST "humidity", BAD_CAST temp);
        }

        snprintf(temp, sizeof(temp), "%d", record.battery);
        if(record.battery != -1) {
            xmlNewChild(metrics, NULL, BAD_CAST "battery", BAD_CAST temp);
        }

        xmlNewChild(entry, NULL, BAD_CAST "timestamp", BAD_CAST record.timestamp);

        snprintf(temp, sizeof(temp), "%d", record.event_code);
        xmlNodePtr event_code = xmlNewChild(entry, NULL, BAD_CAST "event_code", BAD_CAST temp);

        char hexBigEndian[10];
        sprintf(hexBigEndian, "%02X%02X%02X%02X", 0x00, 0x00, 0x00, record.event_code);

        char hexLittleEndian[10];
        sprintf(hexLittleEndian, "%02X%02X%02X%02X", record.event_code, 0x00, 0x00, 0x00);

        //güncel işletim sistemleri little endian kullanır.
        uint32_t littleEndDecimal = (record.event_code) | (0x00 << 8) | (0x00 << 16) | (0x00 << 24);//4 byte veri okunduğu için 32 bitlik sayı kullanılır.
        snprintf(temp, sizeof(temp), "%u", littleEndDecimal);

        xmlNewProp(event_code, BAD_CAST "hexBig", BAD_CAST hexBigEndian);
        xmlNewProp(event_code, BAD_CAST "hexLittle", BAD_CAST hexLittleEndian);
        xmlNewProp(event_code, BAD_CAST "decimal", BAD_CAST temp);
    }

    xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}



void binary_to_xml(const char *outputFile) {
    int keyStart, keyEnd;
    char order[4], dataFileName[100];

    if (read_setup_params("setupParams.json", &keyStart, &keyEnd, order, dataFileName)) {
        return;
    }

    keyStartGlobal = keyStart;
    keyEndGlobal = keyEnd;
    strcpy(orderGlobal, order);
    int recordCount;
    RecordWithKey *records;
    
    if (read_binary_file(dataFileName, keyStart, keyEnd, &recordCount, &records)) {
        return;
    }

    generate_xml(outputFile, records, recordCount);
    free(records);

    printf("XML file generated successfully.\n");
}

void validate_xml(const char *XMLFileName,const char *XSDFileName){
    xmlDocPtr doc;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;
    
    xmlLineNumbersDefault(1); //set line numbers, 0> no substitution, 1>substitution
    ctxt = xmlSchemaNewParserCtxt(XSDFileName); //create an xml schemas parse context
    schema = xmlSchemaParse(ctxt); //parse a schema definition resource and build an internal XML schema
    xmlSchemaFreeParserCtxt(ctxt); //free the resources associated to the schema parser context
    
    doc = xmlReadFile(XMLFileName, NULL, 0); //parse an XML file
    if (doc == NULL)
    {
        fprintf(stderr, "Could not parse %s\n", XMLFileName);
    }
    else
    {
        xmlSchemaValidCtxtPtr ctxt;  //structure xmlSchemaValidCtxt, not public by API
        int ret;
        
        ctxt = xmlSchemaNewValidCtxt(schema); //create an xml schemas validation context 
        ret = xmlSchemaValidateDoc(ctxt, doc); //validate a document tree in memory
        if (ret == 0) //validated
        {
            printf("%s validates\n", XMLFileName);
        }
        else if (ret > 0) //positive error code number
        {
            printf("%s fails to validate\n", XMLFileName);
        }
        else //internal or API error
        {
            printf("%s validation generated an internal error\n", XMLFileName);
        }
        xmlSchemaFreeValidCtxt(ctxt); //free the resources associated to the schema validation context
        xmlFreeDoc(doc);
    }
    // free the resource
    if(schema != NULL)
        xmlSchemaFree(schema); //deallocate a schema structure

    xmlSchemaCleanupTypes(); //cleanup the default xml schemas types library
    xmlCleanupParser(); //cleans memory allocated by the library itself 
    xmlMemoryDump(); //memory dump
}

int main(int argc, char *argv[]) {
    if(argc == 8){

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    const char *conversion_type = argv[3];
    int separator_choice;
    int opsys_choice;
    

    if(strcmp(argv[4], "-separator") == 0) {
        separator_choice = atoi(argv[5]);
    } else {
        printf("Invalid separator argument.\n");
        return 1;
    }

    if (strcmp(argv[6], "-opsys") == 0) {
        opsys_choice = atoi(argv[7]);
    } else {
        printf("Invalid OS argument.\n");
        return 1;
    }
    
    if (strcmp(conversion_type, "1") == 0) {
        csv_to_binary(separator_choice, opsys_choice, input_file, output_file);
    } else if (strcmp(conversion_type, "3") == 0) {
        validate_xml(input_file, output_file);
    } else {
        printf("Invalid conversion type.\n");
        return 1;
    }
    
    }

    else if(argc == 7){
        const char *output_file = argv[1];
        const char *conversion_type = argv[2];
        int separator_choice;
        int opsys_choice;
        if(strcmp(conversion_type, "2") == 0){
            if(strcmp(argv[3], "-separator") == 0) {
                separator_choice = atoi(argv[4]);
            } else {
                printf("Invalid separator argument.\n");
                return 1;
            }
            if (strcmp(argv[5], "-opsys") == 0) {
                opsys_choice = atoi(argv[6]);
            } else {
                printf("Invalid OS argument.\n");
                return 1;
            }
            binary_to_xml(output_file);
        }
    }

    else if(argc < 3){
        if (strcmp(argv[1], "-h") == 0) {
            printf("Usage: ./deviceTool <input_file> <output_file> <conversion_type> -separator <1|2|3> -opsys <1|2|3> [-h]\n");
            return 0;
        }
    }

    //compile komutu
    //gcc devicelogs.c -o deviceTool -I/usr/include/libxml2 -lxml2 -ljson-c

    //run komutu örnekleri
    //./deviceTool smartlogs.csv logdata.dat 1 -separator 1 -opsys 2
    //./deviceTool smartlogs.xml 2 -separator 1 -opsys 2
    //./deviceTool smartlogs.xml smartlogs.xsd 3 -separator 1 -opsys 2
    //./deviceTool -h
    return 0;
}
