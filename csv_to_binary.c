#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

int main(int seperator_choice, int opsys_choice){
    FILE *csvFile = fopen("smartlogs.csv","r");
    FILE *datFile = fopen("logdata.dat","wb");//With wb, we can open a file for writing in binary system.

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

    return 0;
}
