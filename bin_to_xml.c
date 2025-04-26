#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct {
    char *data;
    int key;
} Record;

int compare_records(const void *a, const void *b) {
    Record *recordA = (Record *)a;
    Record *recordB = (Record *)b;
    
    if (strcmp(recordA->order, "ASC") == 0) {
        return recordA->key - recordB->key;
    } else {
        return recordB->key - recordA->key;
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

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int recordSize = keyEnd - keyStart;
    *recordCount = file_size / recordSize;
    *records = malloc(sizeof(Record) * (*recordCount));

    for (int i = 0; i < *recordCount; i++) {
        (*records)[i].data = malloc(recordSize);
        fread((*records)[i].data, 1, recordSize, file);

        (*records)[i].key = 0;
        for (int j = keyStart; j < keyEnd; j++) {
            (*records)[i].key = (*records)[i].key * 256 + (unsigned char)(*records)[i].data[j];
        }
    }

    fclose(file);
    return 0;
}

void generate_xml(const char *filename, Record *records, int recordCount) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_element = xmlNewNode(NULL, BAD_CAST "Records");
    xmlDocSetRootElement(doc, root_element);

    for (int i = 0; i < recordCount; i++) {
        xmlNodePtr record_element = xmlNewChild(root_element, NULL, BAD_CAST "Record", NULL);
        xmlNewChild(record_element, NULL, BAD_CAST "Key", BAD_CAST "");
        xmlNewChild(record_element, NULL, BAD_CAST "Data", BAD_CAST records[i].data);
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

    int recordCount;
    Record *records;
    if (read_binary_file(dataFileName, keyStart, keyEnd, &recordCount, &records)) {
        return 1;
    }

    qsort(records, recordCount, sizeof(Record), compare_records);

    generate_xml("output.xml", records, recordCount);

    for (int i = 0; i < recordCount; i++) {
        free(records[i].data);
    }
    free(records);

    printf("XML file generated successfully.\n");
    return 0;
}
