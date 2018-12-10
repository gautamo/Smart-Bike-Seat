#include <EEPROM.h>

// Read parameters from EEPROM or Serial
void readCredentials()
{
    int ssidAddr = 0;
    int passAddr = ssidAddr + SSID_LEN;
    int connectionStringAddr = passAddr + SSID_LEN;


    ssid = "JJ's Phone";
    pass = "giga9203";
    connectionString = "HostName=cs190-jj.azure-devices.net;DeviceId=cs190jj;SharedAccessKey=IeeQetIzx+K43LR4KngaQppEOwpjROwdg5YCvoROTMs=";

}

bool needEraseEEPROM()
{
    char result = 'n';
    readFromSerial("Do you need re-input your credential information?(Auto skip this after 5 seconds)[Y/n]", &result, 1, 5000);
    if (result == 'Y' || result == 'y')
    {
        clearParam();
        return true;
    }
    return false;
}

// reset the EEPROM
void clearParam()
{
    char data[EEPROM_SIZE];
    memset(data, '\0', EEPROM_SIZE);
    EEPROMWrite(0, data, EEPROM_SIZE);
}

#define EEPROM_END 0
#define EEPROM_START 1
void EEPROMWrite(int addr, char *data, int size)
{
    EEPROM.begin(EEPROM_SIZE);
    // write the start marker
    EEPROM.write(addr, EEPROM_START);
    addr++;
    for (int i = 0; i < size; i++)
    {
        EEPROM.write(addr, data[i]);
        addr++;
    }
    EEPROM.write(addr, EEPROM_END);
    EEPROM.commit();
    EEPROM.end();
}

// read bytes from addr util '\0'
// return the length of read out.
int EEPROMread(int addr, char *buf)
{
    EEPROM.begin(EEPROM_SIZE);
    int count = -1;
    char c = EEPROM.read(addr);
    addr++;
    if (c != EEPROM_START)
    {
        return 0;
    }
    while (c != EEPROM_END && count < EEPROM_SIZE)
    {
        c = (char)EEPROM.read(addr);
        count++;
        addr++;
        buf[count] = c;
    }
    EEPROM.end();
    return count;
}
