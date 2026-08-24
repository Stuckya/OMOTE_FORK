#include "protoCodec.h"
#include <cstring>
#include "applicationInternal/omote_log.h"
#include "applicationInternal/hardware/arduinoLayer.h"

namespace Hub {

namespace {

void copyStringField(char* field, size_t fieldSize, const std::string& value) {
    if (fieldSize == 0) {
        return;
    }

    strncpy(field, value.c_str(), fieldSize - 1);
    field[fieldSize - 1] = '\0';
}

void copyRemoteEventData(omote_RemoteEvent& event, const uint8_t* data, size_t dataLen) {
    if (data == nullptr || dataLen == 0) {
        return;
    }

    const size_t bytesToCopy = (dataLen < sizeof(event.data.bytes)) ? dataLen : sizeof(event.data.bytes);
    memcpy(event.data.bytes, data, bytesToCopy);
    event.data.size = bytesToCopy;
}

}

omote_RemoteEvent ProtoCodec::createSleepTimerEvent(omote_OmoteCommand command,
                                                    uint16_t minutes) {
    omote_RemoteEvent event = createRemoteEvent(HUB_DEVICE_ID, command);
    if (command != omote_OmoteCommand_SLEEP_TIMER_CANCEL) {
        event.has_sleep_timer = true;
        event.sleep_timer.minutes = minutes;
    }
    return event;
}

omote_OmoteCommand ProtoCodec::stringToCommand(const std::string& cmd) {
    struct CommandMapping {
        const char* name;
        omote_OmoteCommand command;
    };

    static const CommandMapping commandMappings[] = {
        {"POWER_ON", omote_OmoteCommand_POWER_ON},
        {"POWER_OFF", omote_OmoteCommand_POWER_OFF},
        {"DOWN", omote_OmoteCommand_DOWN},
        {"UP", omote_OmoteCommand_UP},
        {"RIGHT", omote_OmoteCommand_RIGHT},
        {"LEFT", omote_OmoteCommand_LEFT},
        {"SELECT", omote_OmoteCommand_SELECT},
        {"HOME", omote_OmoteCommand_HOME},
        {"MENU", omote_OmoteCommand_MENU},
        {"PLAY_PAUSE", omote_OmoteCommand_PLAY_PAUSE},
        {"VOL_PLUS", omote_OmoteCommand_VOL_PLUS},
        {"VOL_MINUS", omote_OmoteCommand_VOL_MINUS},
        {"VOL_MUTE", omote_OmoteCommand_VOL_MUTE},
        {"SKIP_BACKWARD", omote_OmoteCommand_SKIP_BACKWARD},
        {"SKIP_FORWARD", omote_OmoteCommand_SKIP_FORWARD},
        {"PAIRING_START", omote_OmoteCommand_PAIRING_START},
        {"PAIRING_SUBMIT_PIN", omote_OmoteCommand_PAIRING_SUBMIT_PIN},
        {"PAIRING_CANCEL", omote_OmoteCommand_PAIRING_CANCEL},
        {"SYNC_STATE", omote_OmoteCommand_SYNC_STATE},
        {"STOP", omote_OmoteCommand_STOP},
        {"REWIND", omote_OmoteCommand_REWIND},
        {"FORWARD", omote_OmoteCommand_FORWARD},
        {"CONF", omote_OmoteCommand_CONF},
        {"INFO", omote_OmoteCommand_INFO},
        {"OK", omote_OmoteCommand_OK},
        {"BACK", omote_OmoteCommand_BACK},
        {"SRC", omote_OmoteCommand_SRC},
        {"CHANNEL_UP", omote_OmoteCommand_CHANNEL_UP},
        {"REC", omote_OmoteCommand_REC},
        {"CHANNEL_DOWN", omote_OmoteCommand_CHANNEL_DOWN},
        {"RED", omote_OmoteCommand_RED},
        {"GREEN", omote_OmoteCommand_GREEN},
        {"YELLOW", omote_OmoteCommand_YELLOW},
        {"BLUE", omote_OmoteCommand_BLUE},
        {"GUI_EVENT", omote_OmoteCommand_GUI_EVENT},

        {"SOURCE", omote_OmoteCommand_SRC},
        {"MUTE_TOGGLE", omote_OmoteCommand_VOL_MUTE},
        {"RETURN", omote_OmoteCommand_BACK},
        {"EXIT", omote_OmoteCommand_BACK},
        {"KEY_A", omote_OmoteCommand_RED},
        {"KEY_B", omote_OmoteCommand_GREEN},
        {"KEY_C", omote_OmoteCommand_YELLOW},
        {"KEY_D", omote_OmoteCommand_BLUE},
    };

    for (const auto& mapping : commandMappings) {
        if (cmd == mapping.name) {
            return mapping.command;
        }
    }

    if (!cmd.empty()) omote_log_w("ProtoCodec: unmapped hub command '%s' -> UNSPECIFIED\n", cmd.c_str());
    return omote_OmoteCommand_OMOTE_COMMAND_UNSPECIFIED;
}

omote_OmoteCommandType ProtoCodec::stringToCommandType(const std::string& type) {
    if (type == "SHORT") return omote_OmoteCommandType_SHORT;
    if (type == "LONG") return omote_OmoteCommandType_LONG;
    return omote_OmoteCommandType_SHORT;
}

omote_RemoteEvent ProtoCodec::createRemoteEvent(
    const std::string& device,
    omote_OmoteCommand command,
    omote_OmoteCommandType type,
    const std::string& remote_id,
    const uint8_t* data,
    size_t data_len
) {
    omote_RemoteEvent event = omote_RemoteEvent_init_zero;

    copyStringField(event.device, sizeof(event.device), device);
    event.command = command;
    event.type = type;
    copyStringField(event.remote_id, sizeof(event.remote_id), remote_id);
    copyRemoteEventData(event, data, data_len);

    return event;
}

size_t ProtoCodec::encodeRemoteEvent(const omote_RemoteEvent& event, uint8_t* buffer, size_t buffer_size) {
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    bool status = pb_encode(&stream, omote_RemoteEvent_fields, &event);
    
    if (!status) {
        return 0;
    }
    
    return stream.bytes_written;
}

bool ProtoCodec::decodeCommandResult(const uint8_t* buffer, size_t buffer_size, omote_CommandResult& result) {
    omote_CommandResult proto_result = omote_CommandResult_init_zero;

    pb_istream_t stream = pb_istream_from_buffer(buffer, buffer_size);
    if (!pb_decode(&stream, omote_CommandResult_fields, &proto_result)) {
        return false;
    }

    result = proto_result;
    return true;
}

} // namespace Hub
