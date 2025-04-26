#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct {
    char device_id[10];
    char timestamp[20];
    float temperature;
    float humidity;
    char status[10];
    char location[50];
    char alert_level[20];
    int battery;
    char firmware_ver[20];
    int event_code;
} Record;

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
    *recordCount = file_size / recordSize + 50;
    *records = malloc(sizeof(Record) * (55));

    for (int i = 0; i < 55; i++) {
        //&(*records)[i].battery = malloc(recordSize);
        /*
        fread((*records)[i].device_id, 1, 8,file);
        fread((*records)[i].timestamp, 1, 8,file);
        fread(&(*records)[i].temperature, 1, 8,file);
        fread(&(*records)[i].humidity, 1, 8,file);
        fread((*records)[i].status, 1, 8,file);
        fread((*records)[i].location, 1, 8,file);
        fread((*records)[i].alert_level, 1, 8,file);
        fread(&(*records)[i].battery, 1, 8,file);
        fread((*records)[i].firmware_ver, 1, 8,file);
        fread(&(*records)[i].event_code, 1, 8,file);
        
        */
        fread(&(*records)[i], sizeof(Record), 1, file);
    }

    fclose(file);
    return 0;
}

void generate_xml(const char *filename, Record *records, int recordCount) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_element = xmlNewNode(NULL, BAD_CAST "smartlogs");
    xmlDocSetRootElement(doc, root_element);

    for (int i = 0; i < 55; i++) {
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

int main() {
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
    return 0;
}
