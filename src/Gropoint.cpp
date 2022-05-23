#include "Gropoint.h"

#define debugPrintLn(...)                             \
    {                                                 \
        if (this->_debugSerial)                       \
            this->_debugSerial->println(__VA_ARGS__); \
    }
#define debugPrint(...)                             \
    {                                               \
        if (this->_debugSerial)                     \
            this->_debugSerial->print(__VA_ARGS__); \
    }

#define FRAME_TERMINATOR "\r\n"
#define FRAME_TERMINATOR_LENGTH (sizeof(FRAME_TERMINATOR) - 1) // Length of CRLF without '\0' terminator

static const char *CMD_ASK_ADDRESS = "?";
static const char *CMD_INFO = "I";
static const char *CMD_READ_MOISTURE_START = "M";
static const char *CMD_READ_TEMPERATURE_START = "M1";
static const char *CMD_READ_VALUES = "D0";

Gropoint::Gropoint(SDI12 &sdiInstance) : _sdi(sdiInstance)
{
}

Gropoint::Gropoint(SDI12 &sdiInstance, char address) : _sdi(sdiInstance), _address(address)
{
}

void Gropoint::setCustomDelay(void (*f)(unsigned long))
{
    _delay = *f;
}

void Gropoint::setDebugSerial(Stream &stream)
{
    _debugSerial = &stream;
}

char Gropoint::getSavedAddress() const
{
    return _address;
}

char Gropoint::findAddress()
{

    _sdi.begin();
    _delay(500);

    _sendCommand(CMD_ASK_ADDRESS);

    _readLine();

    if ((_frameBuffer[0] >= '0' && _frameBuffer[0] <= '9') ||
        (_frameBuffer[0] >= 'A' && _frameBuffer[0] <= 'Z') ||
        (_frameBuffer[0] >= 'a' && _frameBuffer[0] <= 'z'))
    {
        _address = _frameBuffer[0];
        return _address;
    }
    else
    {
        return '?';
    }

    _sdi.end();
}

void Gropoint::getInfo(char *buffer, uint8_t size)
{
    _sdi.begin();
    _delay(500);

    _sendCommand(CMD_INFO);

    _readLine(1000);

    if (buffer && size > 1)
    {
        strncpy(buffer, _frameBuffer, strlen(_frameBuffer) < size - 1 ? strlen(_frameBuffer) : size - 1);
    }

    _sdi.end();
}

int Gropoint::readMoisture(float * values, size_t values_length)
{
    if(values == nullptr || values_length <= 0) {
        return -1;
    }

    _sdi.begin();
    _delay(250);

    _sendCommand(CMD_READ_MOISTURE_START);

    /**
     * Receive aTTTn response where:
     *  a = address
     *  TTT = number of seconds that reads will be available
     *  n = number of values (sensors)
     * */
    _readLine();

    if (_frameBuffer[0] != _address)
    {
        return -1;
    }

    uint16_t wait_seconds = (_frameBuffer[1] - '0') * 100 + (_frameBuffer[2] - '0') * 10 + (_frameBuffer[3] - '0');
    int values_read = _frameBuffer[4] - '0';
    // wait TTT seconds
    _readLine(wait_seconds * 1000);

    _sendCommand(CMD_READ_VALUES);
    _readLine();

    if (strlen(_frameBuffer) < 1)
    {
        return -2;
    }

    char *ptr = &_frameBuffer[1];
    size_t i = 0;

    while(i < values_length && i < values_read && ptr != nullptr) {
        values[i] = strtod(ptr, &ptr);
        ++i;
    }

    return values_read;
}

int Gropoint::readTemperature(float * values, size_t values_length) {
    if(values == nullptr || values_length <= 0) {
        return -1;
    }

    _sdi.begin();
    _delay(250);

    _sendCommand(CMD_READ_TEMPERATURE_START);

    /**
     * Receive aTTTn response where:
     *  a = address
     *  TTT = number of seconds that reads will be available
     *  n = number of values (sensors)
     * */
    _readLine();

    if (_frameBuffer[0] != _address)
    {
        return -1;
    }

    uint16_t wait_seconds = (_frameBuffer[1] - '0') * 100 + (_frameBuffer[2] - '0') * 10 + (_frameBuffer[3] - '0');
    int values_read = _frameBuffer[4] - '0';

    // wait TTT seconds
    _readLine(wait_seconds * 1000);

    _sendCommand(CMD_READ_VALUES);
    _readLine(5000);

    if (strlen(_frameBuffer) < 1)
    {
        return -2;
    }
    char *ptr = &_frameBuffer[1];
    size_t i = 0;

    while(i < values_length && i < values_read && ptr != nullptr) {
        values[i] = strtod(ptr, &ptr);
        ++i;
    }

    return values_read;
}

void Gropoint::_sendCommand(const char *command)
{
    char cmd[5] = "";

    if (strncmp(command, CMD_ASK_ADDRESS, strlen(CMD_ASK_ADDRESS)) != 0)
    {
        cmd[0] = _address;
    }

    strncat(cmd, command, strlen(command));
    strcat(cmd, "!");

    debugPrintLn(cmd);

    _sdi.sendCommand(cmd);
    _sdi.flush();
}

int Gropoint::_readByte(uint32_t timeout)
{
    int c;
    uint32_t _startMillis = millis();

    do
    {
        c = _sdi.read();
        if (c >= 0)
        {
            return c;
        }

    } while (millis() - _startMillis < timeout);

    return -1; //-1 indicates timeout;
}

size_t Gropoint::_readBytesUntil(char terminator, char *buffer, size_t length, uint32_t timeout)
{
    if (length < 1)
    {
        return 0;
    }

    size_t index = 0;

    while (index < length)
    {
        int c = _readByte(timeout);
        if (c < 0 || c == terminator)
        {
            break;
        }

        // Skip null characters. Why sometimes sensor sent it?
        if (c > 0)
        {
            buffer[index] = static_cast<char>(c);
            ++index;
        }
    }

    if (index < length)
    {
        buffer[index] = '\0';
    }

    return index;
}

size_t Gropoint::_readLine(char *buffer, size_t size, uint32_t timeout)
{
    size_t len = _readBytesUntil(FRAME_TERMINATOR[FRAME_TERMINATOR_LENGTH - 1], buffer, size - 1, timeout);

    if ((FRAME_TERMINATOR_LENGTH > 1) && (buffer[len - (FRAME_TERMINATOR_LENGTH - 1)] == FRAME_TERMINATOR[0]))
    {
        len -= FRAME_TERMINATOR_LENGTH - 1;
    }

    buffer[len] = '\0';

    debugPrintLn(buffer);
    // Without this brief delay 485 probes aren't read well with 485 adapters... Why??
    _delay(50);
    return len;
}

size_t Gropoint::_readLine()
{
    return _readLine(_frameBuffer, FRAME_BUFFER_SIZE);
}

size_t Gropoint::_readLine(uint32_t timeout)
{
    return _readLine(_frameBuffer, FRAME_BUFFER_SIZE, timeout);
}