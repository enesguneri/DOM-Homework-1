#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} DeviceLog;

//void assignStringValue(char **token, char *separator, char *dest);

int main(){
    FILE *csvFile = fopen("smartlogs.csv","r");
    FILE *datFile = fopen("logdata.dat","wb");//With wb, we can open a file for writing in binary system.
    
    if(csvFile == NULL || datFile == NULL){
        printf("File not found.");
    }
    else {
        char line[200];
        char *separator = ",";
        

        while (fgets(line,sizeof(line),csvFile))
        {
            DeviceLog device;
            char *token;


            char *crlf = strpbrk(line,"\r\n");
            if (crlf)
            {
                *crlf = '\n';
            }
            

            //device id
            token = strtok(line,separator);//bu komut ile line belleğe alınır. Daha sonraki null ile yapılan çağrımlarda virgülden sonraki değere geçer.
            if(token != NULL){
                strcpy(device.device_id, token);
            }

            //timestampt
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                strcpy(device.timestamp,token);
            }

            //temperature
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                device.temperature = atof(token);
            }

            //humidity
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                device.humidity = atof(token);
            }
            
            //status
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                strcpy(device.status,token);
            }

            //location
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                strcpy(device.location,token);
            }

            //alert level
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                strcpy(device.alert_level,token);
            }

            //battery
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                device.battery = atoi(token);
            }

            //firmware version
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                strcpy(device.firmware_ver,token);
            }

            //event code
            token = strtok(NULL,separator);
            if (token != NULL)
            {
                device.event_code = atoi(token);
            }

            fwrite(&device, sizeof(device) , 1, datFile);

        }
        

    }

    fclose(csvFile);
    fclose(datFile);

    return 0;
}




