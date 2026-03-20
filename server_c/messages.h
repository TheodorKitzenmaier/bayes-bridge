#include <cstdint>

enum MessageType : uint32_t {
    kInit = 0,
    kStart,
    kQuery,
    kCollect
};

struct RequestHeader {
    uint32_t length;
    MessageType type;
    uint64_t worker_id;
};

struct Init {
    RequestHeader header;
    char input_file[0x80];
    char prior_file[0x80];
    char output_file[0x80];
    char derived_file[0x80];
};

struct StartData {
    RequestHeader header;
    char command[0x100];
};
