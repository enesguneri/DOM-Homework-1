#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h> // <-- Added this for XML schema validation

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

// Typedef for Record (same as DeviceLog)
typedef DeviceLog Record;

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
            device.humidity = (*token != '\0') ? atof(token) : -1.0f;

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

int read_binary_file(const char *filename, int keyStart, int keyEnd, int *recordCount, Record **records) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error opening binary file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

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
        snprintf(tempID, sizeof(tempID), "%d", i + 1);
        xmlNewProp(entry, BAD_CAST "id", BAD_CAST tempID);

        xmlNodePtr device = xmlNewChild(entry, NULL, BAD_CAST "device", NULL);
        xmlNewChild(device, NULL, BAD_CAST "device_id", BAD_CAST records[i].device_id);
        xmlNewChild(device, NULL, BAD_CAST "location", BAD_CAST records[i].location);
        xmlNewChild(device, NULL, BAD_CAST "firmware_ver", BAD_CAST records[i].firmware_ver);

        xmlNodePtr metrics = xmlNewChild(entry, NULL, BAD_CAST "metrics", NULL);
        xmlNewProp(metrics, BAD_CAST "status", BAD_CAST records[i].status);
        xmlNewProp(metrics, BAD_CAST "alert_level", BAD_CAST records[i].alert_level);

        char temp[20];
        snprintf(temp, sizeof(temp), "%f", records[i].temperature);
        xmlNewChild(metrics, NULL, BAD_CAST "temperature", BAD_CAST temp);

        snprintf(temp, sizeof(temp), "%f", records[i].humidity);
        xmlNewChild(metrics, NULL, BAD_CAST "humidity", BAD_CAST temp);

        snprintf(temp, sizeof(temp), "%d", records[i].battery);
        xmlNewChild(metrics, NULL, BAD_CAST "battery", BAD_CAST temp);

        snprintf(temp, sizeof(temp), "%d", records[i].event_code);
        xmlNewChild(entry, NULL, BAD_CAST "event_code", BAD_CAST temp);

        xmlNewChild(entry, NULL, BAD_CAST "timestamp", BAD_CAST records[i].timestamp);
    }

    xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

void binary_to_xml(const char *outputFile) {
    int keyStart, keyEnd;
    char order[4], dataFileName[100];

    if (read_setup_params("setupParams.json", &keyStart, &keyEnd, order, dataFileName)) {
        printf("okundu.\n");
    }

    printf("Key Start: %d\n", keyStart);
    printf("Key End: %d\n", keyEnd);
    printf("Order: %s\n", order);
    printf("Data File Name: %s\n", dataFileName);

    int recordCount;
    Record *records;
    if (read_binary_file(dataFileName, keyStart, keyEnd, &recordCount, &records)) {
        printf("okundu.\n");
    }

    generate_xml(outputFile, records, recordCount);
    free(records);

    printf("XML file generated successfully.\n");
}

int main(int argc, char *argv[]) {
    if(argc == 8){

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    const char *conversion_type = argv[3];
    int separator_choice;
    int opsys_choice = atoi(argv[7]);
    
    
    if (strcmp(conversion_type, "1") == 0) {
        csv_to_binary(separator_choice, opsys_choice, input_file, output_file);
    } else if (strcmp(conversion_type, "3") == 0) {
        //validate_xml(input_file, output_file);
    } else {
        printf("Invalid conversion type.\n");
        return 1;
    }

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
    }
    else if(argc == 7){
        const char *output_file = argv[1];
        const char *conversion_type = argv[2];
        printf("conversion_type: %s\n", conversion_type);
        int separator_choice;
        int opsys_choice;
        if(strcmp(conversion_type, "2") == 0){
            printf("argc: %d\n", argc);
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
            printf("Usage: ./devicelogs <input_file> <output_file> <conversion_type> -separator <1|2|3> -opsys <1|2|3> [-h]\n");
            return 0;
        }
    }

    printf("Conversion completed successfully.\n");

    return 0;
}
