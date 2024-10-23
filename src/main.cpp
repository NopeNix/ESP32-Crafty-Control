#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <map>

// UUIDs from the craftyUuids.ts file
std::map<std::string, std::string> uuidMap = {
    {"ServiceUuid", "00000001-4c45-4b43-4942-265a524f5453"},
    {"MetaDataUuid", "00000002-4c45-4b43-4942-265a524f5453"},
    {"MiscDataUuid", "00000003-4c45-4b43-4942-265a524f5453"},
    {"TemperatureUuid", "00000011-4c45-4b43-4942-265a524f5453"},
    {"SetPointUuid", "00000021-4c45-4b43-4942-265a524f5453"},
    {"BoostUuid", "00000031-4c45-4b43-4942-265a524f5453"},
    {"BatteryUuid", "00000041-4c45-4b43-4942-265a524f5453"},
    {"LedUuid", "00000051-4c45-4b43-4942-265a524f5453"},
    {"ModelUuid", "00000022-4c45-4b43-4942-265a524f5453"},
    {"VersionUuid", "00000032-4c45-4b43-4942-265a524f5453"},
    {"SerialUuid", "00000052-4c45-4b43-4942-265a524f5453"},
    {"HoursOfOperationUuid", "00000023-4c45-4b43-4942-265a524f5453"},
    {"SettingsUuid", "000001c3-4c45-4b43-4942-265a524f5453"},
    {"PowerUuid", "00000063-4c45-4b43-4942-265a524f5453"},
    {"ChargingUuid", "000000a3-4c45-4b43-4942-265a524f5453"},
    {"PowerBoostHeatStateUuid", "00000093-4c45-4b43-4942-265a524f5453"},
    {"BatteryRemainingUuid", "00000153-4c45-4b43-4942-265a524f5453"},
    {"BatteryCapacityUuid", "00000143-4c45-4b43-4942-265a524f5453"},
    {"BatteryDesignCapacityUuid", "00000183-4c45-4b43-4942-265a524f5453"},
    {"DischargeCyclesUuid", "00000163-4c45-4b43-4942-265a524f5453"},
    {"ChargeCyclesUuid", "00000173-4c45-4b43-4942-265a524f5453"},
    {"HardwareIdUuid", "00000033-4c45-4b43-4942-265a524f5453"},
    {"SNHardwareUuid", "00000053-4c45-4b43-4942-265a524f5453"},
    {"BluetoothAddressUuid", "00000042-4c45-4b43-4942-265a524f5453"},
    {"PCBVersionUuid", "00000043-4c45-4b43-4942-265a524f5453"},
    {"CurrentTemperaturePT1000Uuid", "000000f3-4c45-4b43-4942-265a524f5453"},
    {"AdjustedCurrentTemperaturePT1000Uuid", "00000103-4c45-4b43-4942-265a524f5453"},
    {"AccuTemperatureUuid", "00000113-4c45-4b43-4942-265a524f5453"},
    {"AccuTemperatureMinUuid", "00000123-4c45-4b43-4942-265a524f5453"},
    {"AccuTemperatureMaxUuid", "00000133-4c45-4b43-4942-265a524f5453"},
    {"OperatingTimeAccuUuid", "00000013-4c45-4b43-4942-265a524f5453"},
    {"VoltageAccuUuid", "000000b3-4c45-4b43-4942-265a524f5453"},
    {"VoltageMainsUuid", "000000c3-4c45-4b43-4942-265a524f5453"},
    {"VoltageHeatingUuid", "000000d3-4c45-4b43-4942-265a524f5453"},
    {"CurrentAccuUuid", "000000e3-4c45-4b43-4942-265a524f5453"}
};

bool doConnect = false;
bool connected = false;
BLEAddress *pServerAddress;
BLEClient *pClient;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveName()) {
            std::string deviceName = advertisedDevice.getName();
            if (deviceName == "Storz&Bickel" || deviceName == "STORZ&BICKEL") {
                Serial.println("Found target device by name, stopping scan...");
                advertisedDevice.getScan()->stop();
                pServerAddress = new BLEAddress(advertisedDevice.getAddress());
                doConnect = true;
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT); // Assuming LED_BUILTIN is the pin for the LED
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED initially
    delay(2000); // Delay for serial monitor connection
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    Serial.println("Starting BLE scan for 60 seconds...");
    pBLEScan->start(60); // Increase scan time to 60 seconds
}

bool connectToServer() {
    Serial.println("Attempting to connect to server...");
    pClient = BLEDevice::createClient();
    bool connectionResult = pClient->connect(*pServerAddress);
    if (!connectionResult) {
        Serial.println("Failed to connect to the server.");
        return false;
    }
    Serial.println("Connected to server");

    // Turn on LED to indicate successful connection
    digitalWrite(LED_BUILTIN, HIGH);

    // Attempt to retrieve the primary service
    BLERemoteService* pRemoteService = pClient->getService(BLEUUID(uuidMap["ServiceUuid"]));
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find the main service UUID");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found main service");

    // Iterate over all UUIDs and attempt to read
    for (const auto& pair : uuidMap) {
        try {
            Serial.print("Attempting to find characteristic: ");
            Serial.println(pair.first.c_str());
            BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(pair.second));
            if (pRemoteCharacteristic == nullptr) {
                Serial.print("Failed to find characteristic UUID: ");
                Serial.println(pair.first.c_str());
                continue;
            }
            Serial.print("Found characteristic: ");
            Serial.println(pair.first.c_str());

            if (pRemoteCharacteristic->canRead()) {
                std::string value = pRemoteCharacteristic->readValue();
                Serial.print("Value for ");
                Serial.print(pair.first.c_str());
                Serial.print(": ");
                if (pair.first.find("Temperature") != std::string::npos || pair.first.find("Voltage") != std::string::npos) {
                    // Assuming temperature and voltage are stored as 16-bit integers
                    int16_t intValue = *(int16_t*)value.data();
                    Serial.println(intValue);
                } else if (pair.first.find("Serial") != std::string::npos || pair.first.find("Model") != std::string::npos) {
                    // Assuming serial and model are stored as strings
                    Serial.println(value.c_str());
                } else {
                    // Default to printing the raw value as a string
                    Serial.println(value.c_str());
                }
            }
        } catch (const std::exception& e) {
            Serial.print("Exception for UUID ");
            Serial.print(pair.first.c_str());
            Serial.print(": ");
            Serial.println(e.what());
        }
    }
    return true;
}

void loop() {
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
            connected = true;
        } else {
            Serial.println("Failed to connect to the server.");
        }
        doConnect = false;
    }

    if (connected) {
        // Add any periodic tasks you might need here
        delay(10000); // Wait for 10 seconds before next attempt
    }
}
