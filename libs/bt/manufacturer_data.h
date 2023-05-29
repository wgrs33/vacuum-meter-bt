#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

class ManufacturerData {
public:
    static ManufacturerData* getSingleton() {
        if (_singleton == nullptr)
            _singleton = new ManufacturerData();
        return _singleton;
    }

    const std::string& manufacturer_name() {
        return _manufacturer_name;
    }

    const std::string& model_number() {
        return _model_number;
    }

    const std::string& hardware_revision() {
        return _hardware_revision;
    }

    const std::string& firmware_revision() {
        return _firmware_revision;
    }

    const std::string& software_revision() {
        return _software_revision;
    }

    const std::string& serial_number() {
        return _serial_number;
    }

    const std::string& system_id() {
        return _system_id;
    }

private:
    ManufacturerData() :
        _manufacturer_name("wgrs33"),
        _model_number("vm01"),
        _hardware_revision("1.0"),
        _firmware_revision(PICO_SDK_VERSION_STRING),
        _software_revision(SOFTWARE_REVISION_STRING) {
        pico_unique_board_id_t board_id;
        pico_get_unique_board_id(&board_id);
        _system_id = uint8_to_hex_string(board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
        _serial_number = "vm0001-" + _system_id;
    }

    std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < s; i++) {
            ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
        }
        return ss.str();
    }

    static ManufacturerData *_singleton;
    std::string _manufacturer_name;
    std::string _model_number;
    std::string _serial_number;
    std::string _hardware_revision;
    std::string _firmware_revision;
    std::string _software_revision;
    std::string _system_id;
};

ManufacturerData* ManufacturerData::_singleton = nullptr;